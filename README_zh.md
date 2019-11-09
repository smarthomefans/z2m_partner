# z2m zigbee网关

此项目是一个开源zigbee网关项目, 所有的硬件和软件都会开放给大家使用. 请大家遵守[CC-BY-SA-4.0](https://github.com/smarthomefans/z2m_partner/blob/master/CC-BY-SA-4.0)协议.

此硬件模组可以结合[iobroker.zigbee](https://github.com/ioBroker/ioBroker.zigbee)项目, [zigbee2mqtt](https://github.com/Koenkk/zigbee2mqtt)项目, [Zigbee to Tasmota](https://github.com/arendst/Tasmota/wiki/Zigbee)项目或者其他开源的基于TI CC2530 zigbee SoC的项目使用.

其他有关这个项目的内容, 可以从[ioBroker China](https://bbs.iobroker.cn/)论坛的[此帖子](https://bbs.iobroker.cn/thread-361-1-1.html)获得, 欢迎大家来一起玩智能家居.

## 声明

- **此项目还处于早期版本**.
- EDA使用[kicad](http://www.kicad-pcb.org/)开发, 不存在软件版权问题.
- DIY硬件有风险, 请大家注意用电安全. 如有风险, 后果自负.

## 项目特点

- 使用[E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123) zigbee模组, 模组使用CC2592PA, 发射功率可以达到21dBm. 也可以使用[E18-2G4Z27SI](http://www.ebyte.com/en/product-view-news.aspx?id=522)替换pin2pin兼容的[E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123)模组, 如何设置和实测信号发射功率请参考[此链接](https://github.com/smarthomefans/z2m_partner/blob/master/documents/signal.md).
- 模块自带ESP8266模组. 可以通过WiFi转串口接入智能家居系统, 不再受到主机摆放位置的约束而导致信号覆盖差. 而且可以配合使用[Zigbee to Tasmota](https://github.com/arendst/Tasmota/wiki/Zigbee)项目, 完全不需要上位机, 自己本身就是一个zigbee-wifi网关.
- ESP8266模组和E18模组的CC-DEBUG程序下载引脚连接, 可以通过[CCLib](https://github.com/s-hadinger/CCLib)项目, 直接给E18 zigbee模组下载固件, 不需要另外购买CC-DEBUG在线仿真器.
- 可以通过拨码开关配置多种串口连接方式:
  - USB TTY <==> ESP 8266 TTY. 用于ESP模组和Zigbee模组下载固件.
  - USB TTY <==> E18 TTY. USB直连Zigbee模组, 上位机配置更加简单, 稳定性更强.
  - ESP 8266 TTY <==> E18 TTY. Connect though TTY. ESP模组和Zigbee模组对接, 可以刷WiFi串口固件, 实现WiFi接入上位机智能家居系统, 具体方法参考[此链接](https://github.com/smarthomefans/z2m_partner/blob/master/documents/wifi_serial.md). 或者直接配合[Zigbee to Tasmota](https://github.com/arendst/Tasmota/wiki/Zigbee)项目做独立的Zigbee网关.
  - USB TTY <==> ESP 8266 debug port. ESP 8266 TTY <==> E18 TTY. 和上一种连接方式一样, 多增加了一个debug串口.
- 使用USB **Type-C**接口. 紧跟时代潮流.
- 不管你怎么看, 我认为这个比USB dongle要好看几个数量级. 颜值即正义.

## 硬件图片

![top view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/front_hw.png)
![bottom view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/back_hw.png)

## 3D打印外壳

![cave](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/3Dfinal.jpg)

## PCB预览

![top view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/top_view.png)

![bottom view](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/bottom_view.png)

## BOM单

### 做WiFi/USB有线接入的Zigbee网关

名称|封装|数值|数量|标号|参考报价(元)
---|---|---|---|---|---
贴片钽电容|TypeB|10uF|2|C5, C6|0.69
贴片电容|0603|100nF|3|C3, C4, C7|0.04
贴片LED灯|1206|LED|1|D1|0.30
自恢复限流保险|1812|MF-MSMF050|1|F1|0.22
贴片电阻|0603|5.1k|2|R1, R2|0.004
贴片电阻|0603|10k|6|R3, R4, R7, R10-R12|0.016
贴片电阻|0603|0|1|R8|0.003
贴片电阻|0603|1k|1|R9|0.02
拨码开关|SMT_P1.27mm|2位|1|SW1|2.87
微动开关|SW_SPST_PTS810|SW_Push|2|SW2, SW3|1.21
USB串口芯片|SOP16|CH340C|1|U1|2.85
模拟开关芯片|SOP16|CD4052B|1|U2|1.56
Zigbee模块|E18-MS1PA1-IPX|E18-MS1PA1-IPX|1|U3|27.69
模拟开关芯片|SOT-353|74LVC1G66|2|U4, U5|0.58
低压差线性稳压器|SOT-223-3|1117-33|1|U6|0.329
WiFi模块|ESP-WROOM-02|ESP-WROOM-02|1|U7|13.20
SMA转I-PEX|||1||2.80
TypeC座|||1||0.91

参考价格来自立创商城. 加上PCB成本, 总计成本约**61.236**元.

### 只做USB接入的Zigbee网关

名称|封装|数值|数量|标号|参考报价(元)
---|---|---|---|---|---
贴片钽电容|TypeB|10uF|2|C5, C6|0.69
贴片电容|0603|100nF|2|C3, C7|0.04
贴片LED灯|1206|LED|1|D1|0.30
自恢复限流保险|1812|MF-MSMF050|1|F1|0.22
贴片电阻|0603|5.1k|2|R1, R2|0.004
贴片电阻|0603|10k|3|R3, R4, R7|0.016
贴片电阻|0603|0|1|R6|0.003
贴片电阻|0603|1k|1|R9|0.02
拨码开关|SMT_P1.27mm|2位|1|SW1|2.87
微动开关|SW_SPST_PTS810|SW_Push|1|SW2|1.21
USB串口芯片|SOP16|CH340C|1|U1|2.85
模拟开关芯片|SOP16|CD4052B|1|U2|1.56
Zigbee模块|E18-MS1PA1-IPX|E18-MS1PA1-IPX|1|U3|27.69
低压差线性稳压器|SOT-223-3|1117-33|1|U6|0.329
SMA转I-PEX|||1||2.80
TypeC座|||1||0.91

参考价格来自立创商城. 加上PCB成本, 总计成本约**45.578**元.

### 只做Zigbee router

名称|封装|数值|数量|标号|参考报价(元)
---|---|---|---|---|---
贴片钽电容|TypeB|10uF|2|C5, C6|0.69
贴片电容|0603|100nF|1|C7|0.04
贴片LED灯|1206|LED|1|D1|0.30
自恢复限流保险|1812|MF-MSMF050|1|F1|0.22
贴片电阻|0603|5.1k|2|R1, R2|0.004
贴片电阻|0603|10k|1|R7|0.016
贴片电阻|0603|0|1|R6|0.003
贴片电阻|0603|1k|1|R9|0.02
微动开关|SW_SPST_PTS810|SW_Push|1|SW2|1.21
Zigbee模块|E18-MS1PA1-IPX|E18-MS1PA1-IPX|1|U3|27.69
低压差线性稳压器|SOT-223-3|1117-33|1|U6|0.329
SMA转I-PEX|||1||2.80
TypeC座|||1||0.91

参考价格来自立创商城. 加上PCB成本, 总计成本约**38.226**元.

## 改版计划

1. USB串口芯片自动控制ESP flash和reset引脚, 无需再按键.
2. LED灯交给ESP芯片控制, 可调节开关和显示效果.
3. 优化拨码开关的丝印提示.
4. 可能更新CC2538模组(30%可能性吧).

## 致谢

- 各位大佬[modkam](https://modkam.ru/), [kirovilya](https://github.com/kirovilya), [Koenkk](https://github.com/Koenkk).
- 小F, 帮忙设计“碉堡”的外壳.
- 萝卜哥, 花神以及热爱智能家居的每一位.
