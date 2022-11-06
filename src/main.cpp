#include <Arduino.h>
#include <WiFiManager.h>
#include "config.h"
#include "tgbot.h"
#include "motor.h"

WiFiManager wifiManager;
TGBot bot;
Motor motor;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  Serial.println("Welcome to AlbertGas project!");

  delay(500);

  wifiManager.autoConnect(WIFI_ACCESSPOINT_SSID);
  
  bot.begin();
  motor.begin();
}

void loop() {
  bot.loop();
  motor.loop();
}