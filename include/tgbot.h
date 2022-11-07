#ifndef TGBOT_H__
#define TGBOT_H__

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "config.h"

class TGBot {
    X509List* _cert = nullptr;
    WiFiClientSecure _secured_client;
    UniversalTelegramBot* _bot = nullptr;
    unsigned long _bot_lasttime;                     // last time messages' scan has been done
    
    void handleNewMessages(int numNewMessages);

    void errorUninit();
public:
    TGBot();
    ~TGBot();

    void begin();
    void loop();
};

#endif // TGBOT_H__