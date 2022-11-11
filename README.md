# Remote heating control system

AlbertGas is a DIY project, which aims to remotely control the heating.

The control panel has an encoder with a couple of buttons, however, the minumum functionality only requires rotating encoder.
The environment includes WLAN, which is used for sending remote commands.

## General requirements

- Should be able precisely to control temperature on the heating control panel
- Should be able to receive (and send) remote commands
- Should be secure and safe

## Hardware

- Stepper motor + driver (5V stepper motor 28BYJ-48 + ULN2003 driver)
- ESP8266 (Wemos D1 mini)
- 3D-printed case with mounts

## Software

- waspinator/AccelStepper library for control over stepper motor
- tzapu/WiFiManager library for easy WiFi management (captive portal, WiFi configuration)
- witnessmenow/UniversalTelegramBot library for hosting Telegram Bot

Telegram Bot is a communication layer that provides secure connection to the remote clients.
(This layer would likely later be replaced with the MQTT requests for integration with the Home Automation software).

## Notes

Requires custom modification of the UniversalTelegramBot library for correct reaply_keyboard functionality.

Function _bool UniversalTelegramBot::sendMessageWithReplyKeyboard_ should be adjusted with the following:
Code
```
replyMarkup["keyboard"] = serialized(keyboard);
```

should be replaced with:

```
  if (keyboard.isEmpty())
    replyMarkup["remove_keyboard"] = true;
  else
    replyMarkup["keyboard"] = serialized(keyboard);
```