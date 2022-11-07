#include <Arduino.h>
#include <WiFiManager.h>

#include "config.h"
#include "tgbot.h"

WiFiManager wifiManager;
TGBot bot;

void setup() {
#ifdef ALBERT_DEBUG
  Serial.begin(115200);
  Serial.println(F("Welcome to AlbertGas project!"));
#endif

  delay(500);

  wifiManager.setConnectTimeout(10);
  wifiManager.autoConnect(WIFI_ACCESSPOINT_SSID);

  bot.begin();
}

void loop() {
  bot.loop();
}