// Ser2Net.h
#ifndef _SER2NET_h
#define _SER2NET_h

#include "Module.h"
#include "Ser2NetConfig.pb.h"

#define MODULE_CFG_VERSION 1501 //1501 - 2000

#define PROTOCOL_TCP
#ifndef MAX_NMEA_CLIENTS
#define MAX_NMEA_CLIENTS 1
#endif

#define ZGB_RST_PIN 12
#define TTY_SEL0_PIN 16
#define TTY_SEL1_PIN 5
/*************************  COM Port 0 *******************************/
#define UART_BAUD0 115200 // Baudrate UART0
#ifdef ESP8266
#define SERIAL_PARAM0 SERIAL_8N1 // Data/Parity/Stop UART0
#else
#define SERIAL_PARAM0 SERIAL_8N1 // Data/Parity/Stop UART0
#define SERIAL0_RXPIN 21         // receive Pin UART0
#define SERIAL0_TXPIN 1          // transmit Pin UART0
#endif

#define BUFFERSIZE 1024

class Ser2Net : public Module
{
public:
    File fsUploadFile;
    uint8_t operationFlag = 0; // 0每秒

    Ser2NetConfigMessage config;
    WiFiServer *server;
    WiFiClient *client[MAX_NMEA_CLIENTS];

    uint8_t tmpBuf[BUFFERSIZE];
    uint16_t tmpBufIndex = 0;

    void httpSetting(ESP8266WebServer *server);
    void httpZigbeeDown(ESP8266WebServer *server);
    void httpZigbeeUpload(ESP8266WebServer *server);
    void httpZigbeeUploadCallBack(ESP8266WebServer *server);

    void setMode(bool isNormal);
    int loadZigbee();
    void resetZigbee();
    void ser2net();

    void init();
    String getModuleName() { return F("ser2net"); }
    String getModuleCNName() { return F("Z2M网关"); }
    String getModuleVersion() { return F("2020.03.15.0000"); }
    String getModuleAuthor() { return F("情留メ蚊子"); }
    bool moduleLed();

    void loop();
    void perSecondDo();

    void readConfig();
    void resetConfig();
    void saveConfig(bool isEverySecond);

    void httpAdd(ESP8266WebServer *server);
    void httpHtml(ESP8266WebServer *server);
    String httpGetStatus(ESP8266WebServer *server);
};
#endif