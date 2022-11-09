#include "tgbot.h"

TGBot::TGBot() {
    _cert = new X509List(TELEGRAM_CERTIFICATE_ROOT);
    _bot = new UniversalTelegramBot(TGBOT_TOKEN, _secured_client);
    _timerGetUpdates = new TimerMs(TGBOT_UPDATES_INTERVAL);
}
TGBot::~TGBot() {
    delete _cert;
    delete _bot;
    delete _timerGetUpdates;
}

void TGBot::begin() {
    if (!_bot || !_cert) {
        return errorUninit();
    }
    
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

    _bot->longPoll = 60;

    // Initialize bot command handlers map
    _handlers.insert({"/help", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleHelp(msg, args); }});
    _handlers.insert({"/start", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStart(msg, args); }});
    _handlers.insert({"/set", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleSet(msg, args); }});
    _handlers.insert({"/move", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleMove(msg, args); }});
    _handlers.insert({"/warmup", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleWarmupCooldown(msg, args); }});
    _handlers.insert({"/cooldown", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleWarmupCooldown(msg, args); }});
    _handlers.insert({"/status", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStatus(msg, args); }});

    // Initialize timer for telegram updates
    _timerGetUpdates->setPeriodMode();
    _timerGetUpdates->start();

    // Motor
    _motor.begin();
}

void TGBot::loop() {
    if (!_bot) {
        return errorUninit();
    }
    _motor.loop();

    if (!_motor.isRunning()) {
        if (_motor.hasJustFinishedTrajectory() && !_trajRequestId.isEmpty()) {
            Serial.println(F("Done"));
            String response = F("Done in ") + String((millis() - _motor.getTraj().timestamp) / 1000.) + F(" seconds");
            _bot->sendMessage(_trajRequestId, response);
            _trajRequestId.clear();
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
        }
    }
}

void TGBot::setTemperature(int temp) {
    _curTemp = temp;

    // First rotate to the minimum temperature
    float firstRotation = -CP_FULLSTOP_ENCODER_STEPS * CP_ANGLE_OF_CLICK;
    // Then rotate to achieve correct temperature
    float secondRotation = (temp - CP_MIN_TEMP) * CP_ANGLE_OF_CLICK;

    MTrajectory traj = MTrajectory({firstRotation, secondRotation});
    _motor.moveTrajectory(MTrajectory({firstRotation, secondRotation}));
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
    _bot->sendMessage(msg.chat_id, F("I don't understand!"));
}
void TGBot::cmdHandleHelp(const telegramMessage& msg, const std::vector<String>& args) {
    _bot->sendMessage(msg.chat_id, F(
"/help - this message\n/start - show welcome message\n/set [int]temp - set current temperature to temp\n/warmup [int]temp - add temp degrees\n/cooldown [int]temp - remove temp degrees\n/move (rel|abs) [float]ang - move motor to ang angle in absolute or relative units\n/status - show status\n"
    ));
}
void TGBot::cmdHandleStart(const telegramMessage& msg, const std::vector<String>& args) {
    _bot->sendMessage(msg.chat_id, F("Hey there, it's me! Let's do some cool stuff here!"));
}
void TGBot::cmdHandleSet(const telegramMessage& msg, const std::vector<String>& args) {
    if (args.size() < 2) {
        _bot->sendMessage(msg.chat_id, F("/set requires one positional argument"));
        return;
    }
    int temp = args[1].toInt();
    if (temp < CP_MIN_TEMP || temp > CP_MAX_TEMP) {
        _bot->sendMessage(msg.chat_id, F("Temperature is not in range"));
        return;
    }

#ifdef ALBERT_DEBUG
    Serial.print(F("Setting temperature to "));
    Serial.println(temp);
#endif
    _trajRequestId = msg.chat_id;
    setTemperature(temp);
}
void TGBot::cmdHandleWarmupCooldown(const telegramMessage& msg, const std::vector<String>& args) {
    int dir = args[0] == F("/warmpup") ? -1 : 1;
    if (args.size() < 2) {
        _bot->sendMessage(msg.chat_id, args[0] + F(" requires one positional argument"));
        return;
    }
    if (_curTemp == -274) {
        _bot->sendMessage(msg.chat_id, F("Current temperature unknown. Use /set first."));
        return;
    }
    int temp = args[1].toInt();
    if (_curTemp - temp < CP_MIN_TEMP || _curTemp + temp > CP_MAX_TEMP) {
        _bot->sendMessage(msg.chat_id, F("Current temperature: ") + String(_curTemp) + F(". Final temperature not in range."));
        return;
    }

    int deltaTemp = dir * temp;

#ifdef ALBERT_DEBUG
    Serial.print(F("Warmup/cooldown by "));
    Serial.println(deltaTemp);
#endif

    _trajRequestId = msg.chat_id;
    _curTemp += deltaTemp;
    _motor.move(deltaTemp * CP_ANGLE_OF_CLICK);
}
void TGBot::cmdHandleMove(const telegramMessage& msg, const std::vector<String>& args) {
    if (args.size() < 3) {
        _bot->sendMessage(msg.chat_id, F("/move requires two positional arguments"));
        return;
    }
    const String& mode = args[1];
    if (mode != F("abs") && mode != F("rel")) {
        _bot->sendMessage(msg.chat_id, F("1st positional arg should be one of ['abs', 'rel']"));
        return;
    }
    if (!Util::stringIsFloat(args[2])) {
        _bot->sendMessage(msg.chat_id, F("2nd positional arg should be angle in degrees"));
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
void TGBot::cmdHandleStatus(const telegramMessage& msg, const std::vector<String>& args) {
    String curTempStr = _curTemp == -274 ? F("Current temperature unknown. Use /set first.") : (F("Current temperature: ") + String(_curTemp));
    String lastMovementStr = _motor.getTraj().timestamp == 0 ? F("unknown") : (String((millis() - _motor.getTraj().timestamp) / 1000.) + F(" seconds ago"));
    _bot->sendMessage(msg.chat_id, curTempStr + "\nLast movement: " + lastMovementStr + "\nWiFi strength: " + String(WiFi.RSSI()) + " dBm");
}

void TGBot::errorUninit() {
#ifdef ALBERT_DEBUG
    Serial.println(F("TGBot uninitialized!"));
#endif
}