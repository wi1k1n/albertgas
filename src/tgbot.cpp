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
    Serial.println(F("TGBot - Loaded whitelist:"));
    for (const auto& id : _whitelist)
        Serial.println(id);

    // Time and security routine
    configTime(0, 0, "pool.ntp.org");       // get UTC time via NTP
    _secured_client.setTrustAnchors(_cert);   // Add root certificate for api.telegram.org
    
    // Check NTP/Time, usually it is instantaneous and you can delete the code below.
    Serial.print(F("Retrieving time: "));
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
        Serial.print(F("."));
        delay(100);
        now = time(nullptr);
    }
    Serial.println(now);

    _bot->longPoll = 60;

    // Initialize bot command handlers map
    _handlers.insert({"/help", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleHelp(msg, args); }});
    _handlers.insert({"/start", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStart(msg, args); }});
    _handlers.insert({"/set", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleSet(msg, args); }});
    _handlers.insert({"/stop", [&](const telegramMessage& msg, const std::vector<String>& args) { cmdHandleStop(msg, args); }});

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
    if (_motor.isRunning()) {
        return;
    }
    
    if (_timerGetUpdates->tick()) { // on timer update
        int numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);
        while (numNewMessages)
        {
            Serial.println(F("got response"));
            handleNewMessages(numNewMessages);
            numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);
        }

        Serial.println(F("I will happen much less often with a long poll"));
    }
}

void TGBot::setTemperature(int temp) {
    // TODO: calculate proper motor trajectory
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
    if (temp < 5 || temp > 27) {
        _bot->sendMessage(msg.chat_id, F("Temperature should be in range 5..27"));
        return;
    }
    
    setTemperature(temp);
}
void TGBot::cmdHandleStop(const telegramMessage& msg, const std::vector<String>& args) {
    _motor.stop();
    _bot->sendMessage(msg.chat_id, F("Motor stopped"));
}

void TGBot::errorUninit() {
    Serial.println(F("TGBot uninitialized!"));
}