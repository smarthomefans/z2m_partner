// config: ////////////////////////////////////////////////////////////
#ifndef _ESP_SERIAL_BRIDGE_CONFIG_H_
#define _ESP_SERIAL_BRIDGE_CONFIG_H_

#define PROTOCOL_TCP
#ifndef MAX_NMEA_CLIENTS
#define MAX_NMEA_CLIENTS 1
#endif

unsigned char qos = 1; //subscribe qos
bool retained = false;

#define ZGB_RST_PIN     12
/*************************  COM Port 0 *******************************/
#define UART_BAUD0 115200        // Baudrate UART0
#ifdef ESP8266
#define SERIAL_PARAM0 SERIAL_8N1 // Data/Parity/Stop UART0
#else
#define SERIAL_PARAM0 SERIAL_8N1 // Data/Parity/Stop UART0
#define SERIAL0_RXPIN 21         // receive Pin UART0
#define SERIAL0_TXPIN 1          // transmit Pin UART0
#endif

#define BUFFERSIZE 1024

#endif
