#ifndef TGBOT_H__
#define TGBOT_H__

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "config.h"

class TGBot {
    X509List* cert = nullptr;
    WiFiClientSecure secured_client;
    UniversalTelegramBot* bot = nullptr;
    
    unsigned long bot_lasttime;          // last time messages' scan has been done
    
    void handleNewMessages(int numNewMessages) {
        for (int i = 0; i < numNewMessages; i++) {
            bot->sendMessage(bot->messages[i].chat_id, bot->messages[i].text, "");
        }
    }
public:
    TGBot() {
        cert = new X509List(TELEGRAM_CERTIFICATE_ROOT);
        bot = new UniversalTelegramBot(TGBOT_TOKEN, secured_client);
    }
    ~TGBot() {
        delete cert;
        delete bot;
    }

    void begin() {
        if (!bot || !cert) {
            return;
        }

        configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
        secured_client.setTrustAnchors(cert); // Add root certificate for api.telegram.org
        
        // Check NTP/Time, usually it is instantaneous and you can delete the code below.
        Serial.print("Retrieving time: ");
        time_t now = time(nullptr);
        while (now < 24 * 3600)
        {
            Serial.print(".");
            delay(100);
            now = time(nullptr);
        }
        Serial.println(now);

        bot->longPoll = 60;
    }

    void loop() {
        if (!bot) {
            return;
        }

        if (millis() - bot_lasttime > TGBOT_MTBS)
        {
            int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

            while (numNewMessages)
            {
                Serial.println("got response");
                handleNewMessages(numNewMessages);
                numNewMessages = bot->getUpdates(bot->last_message_received + 1);
            }

            Serial.println("I will happen much less often with a long poll");
            bot_lasttime = millis();
        }
    }
};

#endif // TGBOT_H__