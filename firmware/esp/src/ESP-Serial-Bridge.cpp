#include <Arduino.h>
#include "config.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#include "FS.h"

#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ArduinoJson.h>

void callback(char *topic, byte *payload, unsigned int length);

// For debug log output/update FW
#if (DEBUG)
HardwareSerial *DBGCOM = &Serial1;
#define LOGD(...) do {\
  DBGCOM->printf(__VA_ARGS__);\
  DBGCOM->println();\
} while(0)
#else
#define LOGD(...)
#endif

struct Config {
  char hostname[16];
  char ssid[64];
  char password[64];
  int tcpPort;
  char mqttServer[32];
  int mqttPort;
  char mqttUser[32];
  char mqttPass[32];
  char mqttClientID[32];
  char mqttPubTopic[32];
  char mqttSubTopic[32];
};

struct Config config;

HardwareSerial *COM = &Serial;

uint8_t buf1[BUFFERSIZE];
uint16_t i1 = 0;

uint8_t buf2[BUFFERSIZE];
uint16_t i2 = 0;

#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer *server;
WiFiClient *TCPClient[MAX_NMEA_CLIENTS];
#endif
PubSubClient *client;

AsyncWebServer wifiManagerServer(80);
DNSServer wifiManagerDNS;

void callback(char *topic, byte *payload, unsigned int length)
{

  byte *p = (byte *)malloc(length);
  memcpy(p, payload, length); // Copy the payload to the new buffer
  client->publish(config.mqttPubTopic, p, length, retained);
  COM->write(p, length); // UART write buffer received via MQTT sub_topic
  free(p);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client->connected())
  {
    LOGD("Attempting MQTT connection...");
    // Attempt to connect
    if (client->connect(config.mqttServer, config.mqttUser, config.mqttPass))
    {
      // Once connected, publish an announcement...
      client->publish(config.mqttPubTopic, "UART<-->WiFi Bridge Reconnected", false);
      // ... and resubscribe
      client->subscribe(config.mqttSubTopic, qos);
    }
    else
    {
      LOGD("failed, rc=%d try again in 5 seconds.", client->state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

static inline void reset_zigbee()
{
  digitalWrite(ZGB_RST_PIN, LOW);
  delay(500);
  digitalWrite(ZGB_RST_PIN, HIGH);
  if (strlen(config.mqttServer) && client->connected()) {
    client->publish(config.mqttPubTopic, "ZNP device reset.", false);
  }
}

static void getConfigFromFile() {

#define READ_STR_CONFIG_FROM_JSON(name) \
strlcpy(config.name, doc[#name] | "", sizeof(config.name))
#define READ_INT_CONFIG_FROM_JSON(name) \
config.name = doc[#name] | 0

  StaticJsonDocument<512> doc;
  SPIFFS.begin();
  File f = SPIFFS.open("/config.json", "r");
  if (!f) {
    LOGD("Config file not exist.");
    SPIFFS.end();
    return;
  }
  DeserializationError error = deserializeJson(doc, f);
  if (error) {
    LOGD("Failed to read file, using default configuration");
    SPIFFS.end();
    return;
  }
  READ_STR_CONFIG_FROM_JSON(hostname);
  READ_STR_CONFIG_FROM_JSON(ssid);
  READ_STR_CONFIG_FROM_JSON(password);
  READ_STR_CONFIG_FROM_JSON(mqttServer);
  READ_STR_CONFIG_FROM_JSON(mqttUser);
  READ_STR_CONFIG_FROM_JSON(mqttPass);
  READ_STR_CONFIG_FROM_JSON(mqttClientID);
  READ_STR_CONFIG_FROM_JSON(mqttPubTopic);
  READ_STR_CONFIG_FROM_JSON(mqttSubTopic);
  READ_INT_CONFIG_FROM_JSON(tcpPort);
  READ_INT_CONFIG_FROM_JSON(mqttPort);
  SPIFFS.end();
}

void setup()
{
  delay(500);
#ifdef ESP8266
  COM->begin(UART_BAUD0, SERIAL_PARAM0);
#else
  COM->begin(UART_BAUD0, SERIAL_PARAM0, SERIAL0_RXPIN, SERIAL0_TXPIN);
#endif
#if (DEBUG)
  DBGCOM->begin(115200);
#endif
  LOGD("\n\n WiFi Serial Bridge V2.00");

  getConfigFromFile();

  if (strlen(config.ssid) == 0)
  {
    LOGD("No SSID defined, use wifi manager.");
    AsyncWiFiManager wifiManager(&wifiManagerServer, &wifiManagerDNS);
#if (DEBUG)
    wifiManager.setDebugOutput(false);
#else
    wifiManager.setDebugOutput(false);
#endif
    wifiManager.autoConnect(config.hostname);
  }
  else
  {
    LOGD("SSID defined, connect to %s", config.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    LOGD("Connecting to %s", config.ssid);
  }
  LOGD(WiFi.localIP().toString().c_str());

#ifdef PROTOCOL_TCP
  static WiFiServer server_0(config.tcpPort);
  server = &server_0;
#endif
  static WiFiClient mqc;
  static PubSubClient mqttClient(config.mqttServer, config.mqttPort, callback, mqc);
  client = &mqttClient;

  // Enable MDNS
  MDNS.begin(config.hostname);

  // Enable OTA

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.onStart([]() {
    if (ArduinoOTA.getCommand() == U_FLASH)
      LOGD("Start updating sketch");
    else// U_FS
      LOGD("Start updating filesystem");
  });
  ArduinoOTA.onEnd([]() {
    LOGD("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    LOGD("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    LOGD("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      LOGD("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      LOGD("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      LOGD("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      LOGD("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      LOGD("End Failed");
    }
  });
  ArduinoOTA.begin();

  if (strlen(config.mqttServer) &&
    client->connect(config.mqttServer, config.mqttUser, config.mqttPass)) {
    client->publish(config.mqttPubTopic, "UART<-->WiFi Bridge Connected", false);
    client->publish(config.mqttPubTopic, WiFi.localIP().toString().c_str(), false);
    client->subscribe(config.mqttSubTopic, qos);
  }

#ifdef PROTOCOL_TCP
  LOGD("Starting TCP Server");
  server->begin(); // start TCP server
  server->setNoDelay(true);
#endif

#ifndef ESP8266
  esp_err_t esp_wifi_set_max_tx_power(50); //lower WiFi Power
#endif
  pinMode(ZGB_RST_PIN, OUTPUT);
  digitalWrite(ZGB_RST_PIN, HIGH);
  reset_zigbee();
//  pinMode(12, INPUT);
//  pinMode(13, INPUT);
//  pinMode(14, INPUT);
//  pinMode(16, INPUT);
}

void loop()
{
  if (strlen(config.mqttServer))
  {
    if (!client->connected())
    {
      reconnect();
    }
    client->loop();
  }

#ifdef PROTOCOL_TCP
  for (byte i = 0; i < MAX_NMEA_CLIENTS; i++)
  {
    //find disconnected spot
    if (TCPClient[i] && !TCPClient[i]->connected())
    {
      TCPClient[i]->stop();
      delete TCPClient[i];
      TCPClient[i] = NULL;
      LOGD("Client disconnected");
    }
  }
  if (server->hasClient())
  {
    for (byte i = 0; i < MAX_NMEA_CLIENTS; i++)
    {
      //find free/disconnected spot
      if (!TCPClient[i] || !TCPClient[i]->connected())
      {
        if (TCPClient[i])
        {
          TCPClient[i]->stop();
          delete TCPClient[i];
          TCPClient[i] = NULL;
          LOGD("Client disconnected");
        }
        TCPClient[i] = new WiFiClient;
        *TCPClient[i] = server->available();
        LOGD("New client for COM");
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient TmpserverClient = server->available();
    TmpserverClient.stop();
  }
#endif

  if (COM != NULL)
  {
    for (byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
    {
      if (TCPClient[cln])
      {
        while (TCPClient[cln]->available())
        {
          buf1[i1] = TCPClient[cln]->read(); // read char from TCP port:8880
          if (i1 < BUFFERSIZE - 1)
            i1++;
        }
        if (i1 > 0)
        {
          COM->write(buf1, i1); // now send to UART
#if (DEBUG)
          DBGCOM->printf("TX(%d):\t", i1);
          for (int i = 0; i < i1; i++)
            DBGCOM->printf("%02x ", buf1[i]);
          DBGCOM->println();
#endif
          i1 = 0;
        }
      }
    }

    if (COM->available())
    {
      while (COM->available())
      {
        buf2[i2] = COM->read(); // read char from UART
        if (i2 < BUFFERSIZE - 1)
          i2++;
      }
      // now send to WiFi:
      for (byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
      {
        if (TCPClient[cln])
          TCPClient[cln]->write(buf2, i2); //send the buffer to TCP port:8880
      }
      if (strlen(config.mqttServer))
      {
        client->publish(config.mqttPubTopic, buf2, i2, retained); //Publish the buffer received via serial
      }
#if (DEBUG)
        DBGCOM->printf("RX(%d):\t", i2);
        for (int i = 0; i < i2; i++)
          DBGCOM->printf("%02x ", buf2[i]);
        DBGCOM->println();
#endif
      i2 = 0;
    }
  }

#ifdef ESP8266
  MDNS.update();
#endif
  ArduinoOTA.handle();
}
