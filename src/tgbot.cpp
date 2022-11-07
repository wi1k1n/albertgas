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
    Serial.println(F("TGBot - Loading whitelist:"));
    _whitelist.clear();
    String wl = TGBOT_WHITELIST;
    String curId;
    for (auto it = wl.begin(); it != wl.end(); it++) {
        if (*it == ',') {
            Serial.println(curId);
            _whitelist.insert(std::move(curId));
            curId = String();
            continue;
        }
        curId += *it;
    }

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

    // We should wait while motor is running
    if (_interrupt) {
        _motor.stop();
    } else {
        _motor.loop();
    }
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
    String curToken;
    for (auto it = cmd.begin(); it != cmd.end(); it++) {
        if (*it == ' ') {
            tokens.push_back(curToken);
            curToken = String();
            continue;
        }
        curToken += *it;
    }
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
            _bot->sendMessage(msg.chat_id, F("I don't understand!"));
            continue;
        }
        if (tokens[0] == "/start") {
            _bot->sendMessage(msg.chat_id, F("Hey there, it's me! Let's do some cool stuff here!"));
            continue;
        } else if (tokens[0] == "/help") {
            _bot->sendMessage(msg.chat_id, F("/help - not implemented yet"));
            continue;
        } else if (tokens[0] == "/set") {
            if (tokens.size() < 2) {
                _bot->sendMessage(msg.chat_id, F("/set requires one positional argument"));
                continue;
            }
            int temp = tokens[1].toInt();
            if (temp < 5 || temp > 27) {
                _bot->sendMessage(msg.chat_id, F("temp should be in range 5..27"));
                continue;
            }
            setTemperature(temp);
        }
    }
}

void TGBot::errorUninit() {
    Serial.println(F("TGBot uninitialized!"));
}