#ifndef TGBOT_H__
#define TGBOT_H__

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <TimerMs.h>
#include <set>

#include "config.h"
#include "motor.h"

class TGBot {
    WiFiClientSecure _secured_client;
    X509List* _cert{ nullptr };
    UniversalTelegramBot* _bot{ nullptr };

    TimerMs* _timerGetUpdates{ nullptr };
    std::set<String> _whitelist;
    
    Motor _motor;
    bool _interrupt{ false }; // set to true when user requests to stop motor movement
    
    void handleNewMessages(int numNewMessages);
    void setTemperature(int temp);

    void errorUninit();
public:
    TGBot();
    ~TGBot();

    void begin();
    void loop();
};

#endif // TGBOT_H__