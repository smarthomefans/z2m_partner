#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include "Framework.h"
#include "Ser2Net.h"

Framework espfw;

void setup()
{
    module = new Ser2Net();
    espfw.setup();
}

void loop()
{
    espfw.loop();
}