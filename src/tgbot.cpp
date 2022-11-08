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
    // First rotate to the minimum temperature
    float firstRotation = CP_FULLSTOP_ANGLE;
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
    _bot->sendMessage(msg.chat_id, F("/help - not implemented yet"));
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
    setTemperature(temp);
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