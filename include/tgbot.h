#ifndef TGBOT_H__
#define TGBOT_H__

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
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
    WiFiManager* _wifiManager;
    Motor _motor;
    String _trajRequestId; // Id of the chat where last trajectory was requested from

    TimerMs* _timerGetUpdates{ nullptr };
    std::set<String> _whitelist;
    std::map<String, std::function<void(const telegramMessage&, const std::vector<String>& args)>> _handlers;
    TimerMs* _timerRestartESP{ nullptr };
    
    float _curTemp{ -274 };
    String _keyboard;
    
    void handleNewMessages(int numNewMessages);
    void setTemperature(int temp);
    bool sendMessage(const String& chat_id, const String& text, const String& parse_mode = "");

    void handleDontUnderstand(const telegramMessage& msg);
    void cmdHandleStart(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleHelp(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleSet(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleKeyboard(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleResetWiFi(const telegramMessage& msg, const std::vector<String>& args);

    void cmdHandleStatus(const telegramMessage& msg, const std::vector<String>& args);
    void cmdHandleMove(const telegramMessage& msg, const std::vector<String>& args);

    void errorUninit();
public:
    TGBot();
    ~TGBot();

    void begin(WiFiManager* wifimanager);
    void loop();
};

#endif // TGBOT_H__