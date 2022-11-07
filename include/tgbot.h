#ifndef TGBOT_H__
#define TGBOT_H__

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <TimerMs.h>
#include <set>
#include <map>
#include <functional>

#include "config.h"
#include "util.h"
#include "motor.h"

class TGBot {
    WiFiClientSecure _secured_client;
    X509List* _cert{ nullptr };
    UniversalTelegramBot* _bot{ nullptr };

    TimerMs* _timerGetUpdates{ nullptr };
    std::set<String> _whitelist;
    std::map<String, std::function<void(const telegramMessage&, const std::vector<String>& args)>> _handlers;
    
    Motor _motor;
    bool _interrupt{ false }; // set to true when user requests to stop motor movement
    
    void handleNewMessages(int numNewMessages);
    void setTemperature(int temp);

    void handleDontUnderstand(const telegramMessage& msg);
    void cmdHandleHelp(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleStart(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleSet(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleStop(const telegramMessage& msg, const std::vector<String>& args);

    void errorUninit();
public:
    TGBot();
    ~TGBot();

    void begin();
    void loop();
};

#endif // TGBOT_H__