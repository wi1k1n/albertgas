#include "tgbot.h"

const char TGBOT_HELP_MSG[] PROGMEM = "/start - show welcome message\n/help - show help\n/set [int]temp - set current temperature to temp\n/keyboard ([int]temp{1-4}) - sets arg values as preset temps for the keyboard; if no arguments - removes the keyboard\n/resetwifi - forgets current SSID/password and restarts ESP";

TGBot::TGBot() {
    _cert = new X509List(TELEGRAM_CERTIFICATE_ROOT);
    _bot = new UniversalTelegramBot(TGBOT_TOKEN, _secured_client);
    _timerGetUpdates = new TimerMs(TGBOT_UPDATES_INTERVAL);
    _timerRestartESP = new TimerMs(WIFI_RESTART_DELAY);
}
TGBot::~TGBot() {
    delete _cert;
    delete _bot;
    delete _timerGetUpdates;
}

void TGBot::begin(WiFiManager* wifimanager) {
    if (!_bot || !_cert || !wifimanager) {
        return errorUninit();
    }
    _wifiManager = wifimanager;
    
    // Fill in white list
    _whitelist.clear();
    Util::tokenizeUnique(TGBOT_WHITELIST, ',', _whitelist);
#ifdef ALBERT_DEBUG
    Serial.println(F("TGBot - Loaded whitelist:"));
    for (const auto& id : _whitelist)
        Serial.println(id);
#endif

    // Time and security routine
    configTime(0, 0, "pool.ntp.org");       // get UTC time via NTP
    _secured_client.setTrustAnchors(_cert);   // Add root certificate for api.telegram.org
    
    // Check NTP/Time, usually it is instantaneous and you can delete the code below.
#ifdef ALBERT_DEBUG
    Serial.print(F("Retrieving time: "));
#endif
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
#ifdef ALBERT_DEBUG
        Serial.print(F("."));
#endif
        delay(100);
        now = time(nullptr);
    }
#ifdef ALBERT_DEBUG
    Serial.println(now);
#endif

    _bot->longPoll = TGBOT_LONGPOLL_INTERVAL / 1000;

    // Initialize bot command handlers map
    _handlers.insert({"/start", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStart(msg, args); }});
    _handlers.insert({"/help", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleHelp(msg, args); }});
    _handlers.insert({"/set", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleSet(msg, args); }});
    _handlers.insert({"/keyboard", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleKeyboard(msg, args); }});
    _handlers.insert({"/resetwifi", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleResetWiFi(msg, args); }});
    // For remote debugging purposes
    _handlers.insert({"/status", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStatus(msg, args); }});
    _handlers.insert({"/move", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleMove(msg, args); }});

    // Initialize timer for telegram updates
    _timerGetUpdates->setPeriodMode();
    _timerGetUpdates->start();

    // Motor
    _motor.begin(MOTOR_DIRECTION);
}

void TGBot::loop() {
    if (!_bot) {
        return errorUninit();
    }
    _motor.loop();

    if (_motor.isRunning()) {
        return;
    }

    if (_motor.hasJustFinishedTrajectory() && !_trajRequestId.isEmpty()) {
#ifdef ALBERT_DEBUG
        Serial.println(F("Done in ") + String((millis() - _motor.getTraj().timestamp) / 1000.) + F(" seconds"));
#endif
#if 0
        String response = F("Done in ") + String((millis() - _motor.getTraj().timestamp) / 1000.) + F(" seconds");
        sendMessage(_trajRequestId, response);
#endif
        _trajRequestId.clear();
    }

    if (_timerRestartESP->tick()) {
        _wifiManager->startConfigPortal(WIFI_ACCESSPOINT_SSID);
    }
    
    if (_timerGetUpdates->tick()) { // on timer update
#ifdef ALBERT_DEBUG
        Serial.println(F("Updating TGBot"));
#endif
        int numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);
        if (numNewMessages) {
            handleNewMessages(numNewMessages);
        }
#ifdef ALBERT_DEBUG
        Serial.println(F("Got command or long poll finished"));
#endif
        _timerGetUpdates->restart();
    }
}

void TGBot::setTemperature(int temp) {
    _curTemp = temp;

    // First rotate to the minimum temperature
    float firstRotationAngle = -(float)(CP_MAX_TEMP - CP_MIN_TEMP) / CP_TEMP_PER_CLICK * CP_ANGLE_OF_CLICK;
    // Then rotate to achieve correct temperature
    float secondRotationAngle = ((float)temp - CP_MIN_TEMP) / CP_TEMP_PER_CLICK * CP_ANGLE_OF_CLICK + CP_EXTRA_SHIFT_ANGLE;

    MTrajectory traj = MTrajectory({firstRotationAngle, secondRotationAngle});
    _motor.moveTrajectory(MTrajectory({firstRotationAngle, secondRotationAngle}));
}

bool TGBot::sendMessage(const String& chat_id, const String& text, const String& parse_mode) {
    return _bot->sendMessageWithReplyKeyboard(chat_id, text, parse_mode, _keyboard);
}

std::vector<String> tokenizeCommand(const String& cmd) {
    std::vector<String> tokens;
    Util::tokenizeNonUnique(cmd, ' ', tokens);
    return tokens;
}

void TGBot::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        telegramMessage& msg = _bot->messages[i];
        if (msg.type != F("message")) {
            continue;
        }
        if (_whitelist.find(msg.from_id) == _whitelist.end()) {
            continue;
        }
        if (msg.text[0] != '/') {
            continue;
        }
        if (_timerRestartESP->active()) {
            continue;
        }
        std::vector<String> tokens = tokenizeCommand(msg.text);
        if (!tokens.size()) {
            handleDontUnderstand(msg);
            continue;
        }
        auto cmdHandlerIt = _handlers.find(tokens[0]);
        if (cmdHandlerIt == _handlers.end()) {
            handleDontUnderstand(msg);
            continue;
        }
        cmdHandlerIt->second(msg, tokens);
    }
}



void TGBot::handleDontUnderstand(const telegramMessage& msg) {
    sendMessage(msg.chat_id, F("I don't understand!"));
}
void TGBot::cmdHandleHelp(const telegramMessage& msg, const std::vector<String>& args) {
    sendMessage(msg.chat_id, TGBOT_HELP_MSG);
}
void TGBot::cmdHandleStart(const telegramMessage& msg, const std::vector<String>& args) {
    sendMessage(msg.chat_id, F("Hey there! Let's do some warm stuff here!\n\n") + String(TGBOT_HELP_MSG));
}
void TGBot::cmdHandleSet(const telegramMessage& msg, const std::vector<String>& args) {
    if (args.size() < 2) {
        sendMessage(msg.chat_id, F("/set requires one positional argument"));
        return;
    }
    int temp = args[1].toInt();
    if (temp < CP_MIN_TEMP || temp > CP_MAX_TEMP) {
        sendMessage(msg.chat_id, F("Temperature is not in range"));
        return;
    }

#ifdef ALBERT_DEBUG
    Serial.print(F("Setting temperature to "));
    Serial.println(temp);
#endif
    _trajRequestId = msg.chat_id;
    setTemperature(temp);
}
void TGBot::cmdHandleKeyboard(const telegramMessage& msg, const std::vector<String>& args) {
    if (args.size() < 2) {
        _keyboard.clear();
        sendMessage(msg.chat_id, F("Done"));
        return;
    }
    if (args.size() > 5) {
        sendMessage(msg.chat_id, F("/keyboard can handle no more than four positional arguments"));
        return;
    }
    std::vector<uint8_t> temps;
    for (uint8_t i = 1; i < args.size(); ++i) {
        uint8_t temp = args[i].toInt();
        if (temp < CP_MIN_TEMP || temp > CP_MAX_TEMP) {
            sendMessage(msg.chat_id, F("One of the arguments is not in range"));
            return;
        }
        temps.push_back(temp);
    }
    switch (temps.size()) {
        case 1:
            _keyboard = F("[[\"/set ") + String(temps[0]) + F("\"]]");
            break;
        case 2:
            _keyboard = F("[[\"/set ") + String(temps[0]) + F("\", \"/set ") + String(temps[1]) + F("\"]]");
            break;
        case 3:
            _keyboard = F("[[\"/set ") + String(temps[0]) + F("\", \"/set ") + String(temps[1]) + F("\"], [\"/set ") + String(temps[2]) + F("\"]]");
            break;
        case 4:
            _keyboard = F("[[\"/set ") + String(temps[0]) + F("\", \"/set ") + String(temps[1]) + F("\"], [\"/set ") + String(temps[2]) + F("\", \"/set ") + String(temps[3]) + F("\"]]");
            break;
    }
    sendMessage(msg.chat_id, F("Done"));
}
void TGBot::cmdHandleResetWiFi(const telegramMessage& msg, const std::vector<String>& args) {
    if (!_wifiManager) {
        sendMessage(msg.chat_id, F("Unknown WiFiManager-related issue!"));
        return;
    }
    sendMessage(msg.chat_id, F("ESP restarts in AP config mode in ") + String(WIFI_RESTART_DELAY / 1000) + F(" seconds. Please connect to SSID: ") + String(WIFI_ACCESSPOINT_SSID));
    _timerRestartESP->restart();
    _bot->longPoll = WIFI_RESTART_DELAY / 1000; // to not wait until the long polling finishes
}



void TGBot::cmdHandleStatus(const telegramMessage& msg, const std::vector<String>& args) {
    String curTempStr = _curTemp == -274 ? F("Current temperature unknown. Use /set first.") : (F("Current temperature: ") + String(_curTemp));
    String lastMovementStr = _motor.getTraj().timestamp == 0 ? F("unknown") : (String((millis() - _motor.getTraj().timestamp) / 1000.) + F(" seconds ago"));
    sendMessage(msg.chat_id, curTempStr + "\nLast movement: " + lastMovementStr + "\nWiFi strength: " + String(WiFi.RSSI()) + " dBm");
}
void TGBot::cmdHandleMove(const telegramMessage& msg, const std::vector<String>& args) {
    if (args.size() < 3) {
        sendMessage(msg.chat_id, F("/move requires two positional arguments"));
        return;
    }
    const String& mode = args[1];
    if (mode != F("abs") && mode != F("rel")) {
        sendMessage(msg.chat_id, F("1st positional arg should be one of ['abs', 'rel']"));
        return;
    }
    if (!Util::stringIsFloat(args[2])) {
        sendMessage(msg.chat_id, F("2nd positional arg should be angle in degrees"));
        return;
    }
    float pos = args[2].toFloat();
#ifdef ALBERT_DEBUG
    Serial.print(F("Moving motor to the "));
    Serial.print(mode);
    Serial.println(pos);
#endif
    
    _trajRequestId = msg.chat_id;
    if (mode == F("abs")) {
        _motor.moveTo(pos);
    } else {
        _motor.move(pos);
    }
}

void TGBot::errorUninit() {
#ifdef ALBERT_DEBUG
    Serial.println(F("TGBot uninitialized!"));
#endif
}