#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include "Framework.h"
#include "Ser2Net.h"

void setup()
{
    Framework::one(115200);

    module = new Ser2Net();

    Framework::setup();
}

void loop()
{
    Framework::loop();
}