#include "tgbot.h"

TGBot::TGBot() {
    _cert = new X509List(TELEGRAM_CERTIFICATE_ROOT);
    _bot = new UniversalTelegramBot(TGBOT_TOKEN, _secured_client);
}
TGBot::~TGBot() {
    delete _cert;
    delete _bot;
}

void TGBot::begin() {
    if (!_bot || !_cert) {
        return errorUninit();
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
}

void TGBot::loop() {
    if (!_bot) {
        return errorUninit();
    }

    if (millis() - _bot_lasttime > TGBOT_MTBS)
    {
        int numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);

        while (numNewMessages)
        {
            Serial.println(F("got response"));
            handleNewMessages(numNewMessages);
            numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);
        }

        Serial.println(F("I will happen much less often with a long poll"));
        _bot_lasttime = millis();
    }
}


void TGBot::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        _bot->sendMessage(_bot->messages[i].chat_id, _bot->messages[i].text, "");
    }
}

void TGBot::errorUninit() {
    Serial.println(F("TGBot uninitialized!"));
}