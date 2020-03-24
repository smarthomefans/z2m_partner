#include <EEPROM.h>
#include <TaskScheduler.h>

#include "Framework.h"
#include "Module.h"
#include "Rtc.h"
#include "Http.h"
#include "Util.h"

static Scheduler normal_runner;

uint16_t rebootCount = 0;

#ifndef DISABLE_MQTT
void Framework::callback(char *topic, byte *payload, unsigned int length)
{
    String str;
    for (int i = 0; i < length; i++)
    {
        str += (char)payload[i];
    }

    Debug::AddInfo(PSTR("Subscribe: %s payload: %s"), topic, str.c_str());

    String topicStr = String(topic);
    if (topicStr.endsWith(F("/OTA")))
    {
        Http::OTA(str.endsWith(F(".bin")) ? str : OTA_URL);
    }
    else if (topicStr.endsWith(F("/restart")))
    {
        ESP.reset();
    }
    else if (module)
    {
        module->mqttCallback(topicStr, str);
    }

    Led::led(200);
}

void Framework::connectedCallback()
{
    Mqtt::subscribe(Mqtt::getCmndTopic(F("#")));
    Led::blinkLED(40, 8);
    if (module)
    {
        module->mqttConnected();
    }
}
#endif

static void tickerPerSecondDo()
{
    perSecond++;
    if (perSecond == 30)
    {
        Rtc::rtcReboot.fast_reboot_count = 0;
        Rtc::rtcRebootSave();
    }
    if (rebootCount == 3)
    {
        return;
    }
    Rtc::perSecondDo();

    Config::perSecondDo();
#ifndef DISABLE_MQTT
    Mqtt::perSecondDo();
#endif
    module->perSecondDo();
    LOGD("per second %d", perSecond);
}

void Framework::one(unsigned long baud)
{
    Rtc::rtcRebootLoad();
    Rtc::rtcReboot.fast_reboot_count++;
    Rtc::rtcRebootSave();
    rebootCount = Rtc::rtcReboot.fast_reboot_count > BOOT_LOOP_OFFSET ? Rtc::rtcReboot.fast_reboot_count - BOOT_LOOP_OFFSET : 0;

    Serial.begin(baud);
    EEPROM.begin(GlobalConfigMessage_size + 6);
    globalConfig.debug.type = 1;
}

Task taskPerSecond(1000, TASK_FOREVER, tickerPerSecondDo, &normal_runner, false);
Task taskWiFiLoop(200, TASK_FOREVER, Wifi::loop, &normal_runner, false);
Task taskHttpLoop(100, TASK_FOREVER, Http::loop, &normal_runner, false);
Task taskLedLoop(100, TASK_FOREVER, Led::loop, &normal_runner, false);
#ifndef DISABLE_MQTT
Task taskMqttLoop(100, TASK_FOREVER, Mqtt::loop, &normal_runner, false);
#endif
Task taskRTCLoop(1000, TASK_FOREVER, Rtc::loop, &normal_runner, false);
static void moduleLoop(void) { module->loop(); }
Task taskModuleLoop(100, TASK_FOREVER, moduleLoop, &normal_runner, false);

void Framework::setup()
{
    Debug::AddError(PSTR("---------------------  v%s  %s  -------------------"), module->getModuleVersion().c_str(), Rtc::GetBuildDateAndTime().c_str());
    LOGD("rebootCound %d", rebootCount);
    if (rebootCount == 1)
    {
        // Reset configs
        Config::readConfig();
        module->resetConfig();
    }
    else if (rebootCount == 2)
    {
        // Reset configs
        Config::readConfig();
        module->resetConfig();
    }
    else
    {
        Config::readConfig();
    }

#ifdef FORCE_SERIAL_LOG
    globalConfig.debug.type |= 1;
#endif

    if (globalConfig.uid[0] != '\0')
    {
        strcpy(UID, globalConfig.uid);
    }
    else
    {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        mac = mac.substring(6, 12);
        sprintf(UID, "%s_%s", module->getModuleName().c_str(), mac.c_str());
    }
    Util::strlowr(UID);

    Debug::AddInfo(PSTR("UID: %s"), UID);
    Debug::AddInfo(PSTR("Config Len: %d"), GlobalConfigMessage_size + 6);

    //Config::resetConfig();
    if (MQTT_MAX_PACKET_SIZE == 128)
    {
        Debug::AddError(PSTR("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM OMG LIB FOLDER"));
    }

    taskPerSecond.enable();
    Http::begin();
    Wifi::connectWifi();
    LOGD("%s", "connect wifi down");
    if (rebootCount == 3)
    {
        module = NULL;
    }
    else
    {
#ifndef DISABLE_MQTT
        Mqtt::setClient(Wifi::wifiClient);
        Mqtt::mqttSetConnectedCallback(connectedCallback);
        Mqtt::mqttSetLoopCallback(callback);
#endif
        //module->init();
        Rtc::init();
    }

        taskWiFiLoop.enableIfNot();
        taskHttpLoop.enableIfNot();
        taskLedLoop.enableIfNot();
#ifndef DISABLE_MQTT
        taskMqttLoop.enableIfNot();
#endif
        taskModuleLoop.enableIfNot();
        taskRTCLoop.enableIfNot();
    LOGD("%s", "setup down");
}

void Framework::loop()
{
    normal_runner.execute();
    
    LOGD("%s", "loop debug");
}
