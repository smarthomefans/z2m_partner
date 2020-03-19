#include "Ser2Net.h"
#include "CCLoader.h"
#include <LittleFS.h>

#pragma region 继承

void Ser2Net::init()
{
    pinMode(TTY_SEL0_PIN, OUTPUT);
    pinMode(TTY_SEL1_PIN, OUTPUT);
    setMode(true);

    delay(500);

#ifdef ESP8266
    Serial.begin(UART_BAUD0, SERIAL_PARAM0);
#else
    Serial.begin(UART_BAUD0, SERIAL_PARAM0, SERIAL0_RXPIN, SERIAL0_TXPIN);
#endif

    server = new WiFiServer(config.tcp_port);
    Debug::AddInfo(PSTR("Starting TCP Server Port: %d"), config.tcp_port);
    server->begin(); // start TCP server
    server->setNoDelay(true);

    pinMode(ZGB_RST_PIN, OUTPUT);
    digitalWrite(ZGB_RST_PIN, HIGH);
    resetZigbee();

#ifndef ESP8266
    esp_err_t esp_wifi_set_max_tx_power(50); //lower WiFi Power
#endif

    if (!LittleFS.begin())
    {
        if (!LittleFS.format())
        {
            Debug::AddError(PSTR("LittleFS format failed"));
        }
        else
        {
            if (!LittleFS.begin())
            {
                Debug::AddError(PSTR("LittleFS mount failed"));
            }
        }
    }
}

bool Ser2Net::moduleLed()
{
    return false;
}

void Ser2Net::loop()
{
    ser2net();

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
    }

    if (bitRead(operationFlag, 1))
    {
        bitClear(operationFlag, 1);
        loadZigbee();
    }
}

void Ser2Net::perSecondDo()
{
    bitSet(operationFlag, 0);
}
#pragma endregion

#pragma region 配置

void Ser2Net::readConfig()
{
    globalConfig.debug.type = globalConfig.debug.type & ~1;
    if ((8 & globalConfig.debug.type) == 8)
    {
        Serial1.begin(115200);
    }
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(Ser2NetConfigMessage), Ser2NetConfigMessage_fields, &config);
    if (config.tcp_port == 0)
    {
        config.tcp_port = 8880;
    }
}

void Ser2Net::resetConfig()
{
    globalConfig.debug.type = globalConfig.debug.type & ~1;
    globalConfig.debug.type = globalConfig.debug.type | 8;

    if ((8 & globalConfig.debug.type) == 8)
    {
        Serial1.begin(115200);
    }
    Debug::AddInfo(PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(Ser2NetConfigMessage));

    config.tcp_port = 8880;
}

void Ser2Net::saveConfig(bool isEverySecond)
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, Ser2NetConfigMessage_size, Ser2NetConfigMessage_fields, &config);
}
#pragma endregion

#pragma region HTTP

void Ser2Net::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/setting"), std::bind(&Ser2Net::httpSetting, this, server));
    server->on(F("/zigbee_upload"), HTTP_POST, std::bind(&Ser2Net::httpZigbeeUploadCallBack, this, server), std::bind(&Ser2Net::httpZigbeeUpload, this, server));
    server->on(F("/zigbee_down"), std::bind(&Ser2Net::httpZigbeeDown, this, server));
}

String Ser2Net::httpGetStatus(ESP8266WebServer *server)
{
    return "";
}

void Ser2Net::httpHtml(ESP8266WebServer *server)
{
    String page = F("<form method='post' action='/setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>MQTT设置</th></tr></thead><tbody>");
    page += F("<tr><td>TCP端口</td><td><input type='number' min='0' max='65535' name='tcp_port' required value='{port}'></td></tr>");
    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>保存</button></td></tr>");
    page += F("</tbody></table></form>");
    page.replace(F("{port}"), String(config.tcp_port));

    page += F("<table class='gridtable'><thead><tr><th colspan='2'>ZigBee固件更新</th></tr></thead><tbody>");
    page += F("<form method='POST' action='/zigbee_upload' enctype='multipart/form-data'>");
    page += F("<tr><td colspan='2'><a class='file'><input type='file' name='update'>选择文件</a></td></tr>");
    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>更新</button><br>");
    page += F("</form>");
    page += F("</tbody></table>");

    server->sendContent(page);
}

void Ser2Net::httpSetting(ESP8266WebServer *server)
{
    uint16_t last_tcp_port = config.tcp_port;
    config.tcp_port = server->arg(F("tcp_port")).toInt();
    Config::saveConfig();

    if (config.tcp_port != last_tcp_port)
    {
        server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"修改了重要配置 . . . 正在重启中。\"}"));
        Led::blinkLED(400, 4);
        ESP.restart();
    }
    else
    {
        server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置。\"}"));
    }
}

void Ser2Net::httpZigbeeDown(ESP8266WebServer *server)
{
    File file = LittleFS.open("/2530.bin", "r");
    server->streamFile(file, "application/octet-stream");
    file.close();
}

void Ser2Net::httpZigbeeUploadCallBack(ESP8266WebServer *server)
{
    bitSet(operationFlag, 1);
    server->send_P(200, PSTR("text/html"), PSTR("<meta charset='utf-8'/><meta http-equiv=\"refresh\" content=\"3;URL=/\">上传成功！准备更新ZigBee固件，大概1分钟左右。"));
}

void Ser2Net::httpZigbeeUpload(ESP8266WebServer *server)
{
    HTTPUpload &upload = server->upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        fsUploadFile = LittleFS.open("/2530.bin", "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
            Debug::AddInfo(PSTR("Update Success: %u"), upload.totalSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        Debug::AddInfo(PSTR("Update was aborted"));
    }
    delay(0);
}
#pragma endregion

void Ser2Net::ser2net()
{
    for (uint8_t i = 0; i < MAX_NMEA_CLIENTS; i++)
    {
        //find disconnected spot
        if (client[i] && !client[i]->connected())
        {
            client[i]->stop();
            delete client[i];
            client[i] = NULL;
            Debug::AddInfo(PSTR("Client disconnected"));
        }
    }
    if (server->hasClient())
    {
        for (uint8_t i = 0; i < MAX_NMEA_CLIENTS; i++)
        {
            //find free/disconnected spot
            if (!client[i] || !client[i]->connected())
            {
                if (client[i])
                {
                    client[i]->stop();
                    delete client[i];
                    client[i] = NULL;
                    Debug::AddInfo(PSTR("Client disconnected"));
                }
                client[i] = new WiFiClient;
                *client[i] = server->available();
                Debug::AddInfo(PSTR("New client for COM"));
                continue;
            }
        }
        //no free/disconnected spot so reject
        WiFiClient TmpserverClient = server->available();
        TmpserverClient.stop();
    }

    for (uint8_t cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
    {
        if (!client[cln])
        {
            continue;
        }
        tmpBufIndex = 0;
        while (client[cln]->available())
        {
            tmpBuf[tmpBufIndex++] = client[cln]->read(); // read char from TCP port:8880
        }
        if (tmpBufIndex > 0)
        {
            Serial.write(tmpBuf, tmpBufIndex); // now send to UART
            if ((8 & globalConfig.debug.type) == 8)
            {
                Serial1.printf("TX(%d):\t", tmpBufIndex);
                for (int i = 0; i < tmpBufIndex; i++)
                {
                    Serial1.printf("%02X ", tmpBuf[i]);
                }
                Serial1.println();
            }
        }
    }

    if (Serial.available())
    {
        tmpBufIndex = 0;
        while (Serial.available())
        {
            tmpBuf[tmpBufIndex++] = Serial.read(); // read char from UART
        }
        // now send to WiFi:
        for (uint8_t cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
        {
            if (client[cln])
            {
                client[cln]->write(tmpBuf, tmpBufIndex); //send the buffer to TCP port:8880
            }
        }
        if ((8 & globalConfig.debug.type) == 8)
        {
            Serial1.printf("RX(%d):\t", tmpBufIndex);
            for (int i = 0; i < tmpBufIndex; i++)
            {
                Serial1.printf("%02X ", tmpBuf[i]);
            }
            Serial1.println();
        }
    }
}

void Ser2Net::resetZigbee()
{
    digitalWrite(ZGB_RST_PIN, LOW);
    delay(500);
    digitalWrite(ZGB_RST_PIN, HIGH);
}

void Ser2Net::setMode(bool isNormal)
{
    if (isNormal)
    {
        globalConfig.debug.type = globalConfig.debug.type & ~1;
    }
    else
    {
        globalConfig.debug.type = globalConfig.debug.type | 1;
    }

    digitalWrite(TTY_SEL0_PIN, isNormal ? LOW : HIGH);
    digitalWrite(TTY_SEL1_PIN, isNormal ? HIGH : LOW);
    delay(100);
}

int Ser2Net::loadZigbee()
{
    setMode(false);
    Debug::AddInfo(PSTR("Open 2530.bin"));
    File f = LittleFS.open("/2530.bin", "r");
    if (!f)
    {
        Debug::AddError(PSTR("LittleFS: open 2530.bin error"));
        setMode(true);
        return 2;
    }

    int blkTot = f.size() / 512;
    int remain = f.size() % 512;
    if (remain != 0)
    {
        blkTot++;
        Debug::AddInfo(PSTR("!!WARNING: File's size isn't the integer multiples of 512 bytes, and the last block will be filled in up to 512 bytes with 0xFF!"));
    }
    Debug::AddInfo(PSTR("Block total: %d"), blkTot);

    CCLoader *loader = new CCLoader();
    loader->ProgrammerInit();

    unsigned char chip_id = 0;
    unsigned char debug_config = 0;
    unsigned char verify = 0;

    loader->debug_init();
    chip_id = loader->read_chip_id();
    if (chip_id == 0)
    {
        Debug::AddError(PSTR("No chip detected, run loop again."));
        f.close();
        setMode(true);
        return 3;
    }

    loader->RunDUP();
    loader->debug_init();

    loader->chip_erase();
    loader->RunDUP();
    loader->debug_init();

    // Switch DUP to external crystal osc. (XOSC) and wait for it to be stable.
    // This is recommended if XOSC is available during programming. If
    // XOSC is not available, comment out these two lines.
    loader->write_xdata_memory(DUP_CLKCONCMD, 0x80);
    while (loader->read_xdata_memory(DUP_CLKCONSTA) != 0x80)
    {
        delay(1);
    }
    // Enable DMA (Disable DMA_PAUSE bit in debug configuration)
    debug_config = 0x22;
    loader->debug_command(CMD_WR_CONFIG, &debug_config, 1);

    // Program data (start address must be word aligned [32 bit])
    Led::on();
    unsigned char rxBuf[512];
    unsigned char read_data[512];
    unsigned int addr = 0x0000;
    unsigned char bank;
    unsigned int offset;

    for (uint16_t i = 0; i < blkTot; i++)
    {
        if ((1 & globalConfig.debug.type) == 1)
        {
            Serial.printf("blkTot: %d\r\n", i + 1);
        }
        if ((i == (blkTot - 1)) && (remain != 0))
        {
            f.read(rxBuf, remain);
            int filled = 512 - remain;
            for (uint16_t j = 0; j < filled; j++)
            {
                rxBuf[remain + j] = 0xFF;
            }
            Debug::AddInfo(PSTR("last 0xFF"));
        }
        else
        {
            f.read(rxBuf, 512);
        }

        loader->write_flash_memory_block(rxBuf, addr, 512); // src, address, count
        if (verify)
        {
            bank = addr / (512 * 16);
            offset = (addr % (512 * 16)) * 4;
            loader->read_flash_memory_block(bank, offset, 512, read_data); // Bank, address, count, dest.
            for (uint16_t i = 0; i < 512; i++)
            {
                if (read_data[i] != rxBuf[i])
                {
                    // Fail
                    loader->chip_erase();
                    Debug::AddError(PSTR("Verify Error"));
                    f.close();
                    setMode(true);
                    return 4;
                }
            }
        }
        addr += (unsigned int)128;
        delay(100);
    }
    f.close();

    Led::off();
    loader->RunDUP();

    Debug::AddInfo(PSTR("chip_id %d OK"), chip_id);
    delay(1000);
    setMode(true);
    ESP.restart();
    return 0;
}
