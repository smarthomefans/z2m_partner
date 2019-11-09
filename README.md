# [中文文档](https://github.com/smarthomefans/z2m_partner/blob/master/README_zh.md)

# z2m hardware module

This is hardware module for [iobroker.zigbee](https://github.com/ioBroker/ioBroker.zigbee) and [zigbee2mqtt](https://github.com/Koenkk/zigbee2mqtt) or [Zigbee to Tasmota](https://github.com/arendst/Tasmota/wiki/Zigbee).  
If you can speak Chinese and be interested with this project, please look at this [post](https://bbs.iobroker.cn/thread-361-1-1.html) in [ioBroker China BBS](https://bbs.iobroker.cn/)

## **THIS IS ALPHA VERSION!!!**

## What's the difference

- Use [E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123) zigbee module. 20dBm power can make sure zigbee net work signal is strong.
- Add an ESP8266 module. Give you another option that can connect zigbee module though wifi-to-serial (I was using this [project](https://github.com/smarthomefans/ESP32-Serial-Bridge)). 
If your machine running smarthome platform, such as ioBroker or homeassistant, is put in a hidden corner, for better signal coverage, you can put your zigbee module in some where else.
- ESP8266 connected to E18 firmware download pins, give you a chance that can update E18 though ESP8266, no need to use CC-DEBUG.
- There will be 4 options for different TTY connection:
  - USB TTY <==> ESP 8266 TTY. This option is used for download firmware for E18.
  - USB TTY <==> E18 TTY. If you can connect your module though USB wire, this is more stable.
  - USB TTY <==> ESP 8266 debug port. ESP 8266 TTY <==> E18 TTY. Connect though wifi, and you can capture debug infomation though USB TTY.
  - ESP 8266 TTY <==> E18 TTY. Connect though TTY.

## Hardware module images

![top view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/front_hw.png)
![bottom view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/back_hw.png)

## 3D cover

![cave](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/3Dfinal.jpg)

## PCB preview

![top view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/top_view.png)
![bottom view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/bottom_view.png)

## Thanks to

- [modkam](https://modkam.ru/), [kirovilya](https://github.com/kirovilya), [Koenkk](https://github.com/Koenkk).
