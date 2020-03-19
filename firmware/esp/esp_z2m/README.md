# ESP COVER
**杜亚KT82TV/W窗帘电机个人固件.**

> ### 作者声明
>
> 注意: 本项目主要目的为作者本人自己学习及使用杜亚KT82TV/W窗帘电机而开发，本着开源精神及造福网友而开源，仅个人开发，可能无法做到完整的测试，所以不承担他人使用本项目照成的所有后果。
>
> **严禁他人将本项目用户用于任何商业活动。个人在非盈利情况下可以自己使用，严禁收费代刷等任何盈利服务、**
> 
> 有需要请联系作者：qlwz@qq.com

# 重要说明

- 杜亚电机型号：向日葵KT82TV/W电机
- 原贴：[KT82TV/W WIFI电机硬改ESP MQTT输出 NRF24-MYS 协议](https://bbs.iobroker.cn/t/topic/847)
- PCB：[KT82TV/W WIFI电机硬改ESP MQTT输出（附带pcb）](https://bbs.iobroker.cn/t/topic/852)
- PCB版权所有：Huex

## 特性

本固件使用杜亚KT82TV/W窗帘电机+硬改ESP硬件为基础,实现以下功能:

- [x] 控制窗帘电机
- [x] 自动设置行程
- [x] WEB配置页面
- [x] MQTT服务器连接控制
- [x] 通过MQTT连入Home Assistant


## 如何配网

1、第一次使用自动进入配网模式

2、以后通过长按【按键】进入配网模式

## 如何编译
Visual Studio Code + PlatformIO ID 开发  [安装](https://www.jianshu.com/p/c36f8be8c87f)

## 已支持接入的开源智能家居平台
以下排序随机，不分优劣。合适自己的就好。

### 1、Home Assistant
Home Assistant 是一款基于 Python 的智能家居开源系统，支持众多品牌的智能家居设备，可以轻松实现设备的语音控制、自动化等。
- [官方网站](https://www.home-assistant.io/)
- [国内论坛](https://bbs.hassbian.com/)

#### 接入方法
WEB页面开启**MQTT自动发现**  

### 2、ioBroker
ioBroker是基于nodejs的物联网的集成平台，为物联网设备提供核心服务、系统管理和统一操作方式。
- [官方网站](http://www.iobroker.net)
- [中文资料可以参考这里](https://doc.iobroker.cn/#/_zh-cn/)
- [国内论坛](https://bbs.iobroker.cn)
#### 接入方法
ioBroker相关接入问题可以加QQ群776817275咨询

### 3、其他支持mqtt的平台
理论上来说，只要是支持MQTT的平台都可以实现接入。

#### 接入方法
添加对应的topic

# 固件截图

![image](https://github.com/qlwz/esp_cover/blob/master/file/images/tab1.png?raw=true)
![image](https://github.com/qlwz/esp_cover/blob/master/file/images/tab2.png?raw=true)
![image](https://github.com/qlwz/esp_cover/blob/master/file/images/tab3.png?raw=true)
![image](https://github.com/qlwz/esp_cover/blob/master/file/images/tab4.png?raw=true)



## 拆机接线及烧录固件相关

### 接线方法

PCB板有标明RX TX GPIO0

### 工具/固件下载

确认硬件连接正常后,下载以下软件:

烧录软件: flash_download_tools_vX.zip	[点这里下载](https://www.espressif.com/zh-hans/support/download/other-tools)

完整固件: cover.bin	[点这里下载](https://github.com/qlwz/esp_cover/releases)

### 开始烧录

将flash_download_tools_vX.zip解压,打开目录下的flash_download_tools_vX.exe,选择ESP8266 DownloadTool,根据以下截图做配置,

![image](https://github.com/qlwz/esp_dc1/blob/master/file/flash/flash1.png?raw=true)

将与主控板连接的usbTTL连接上电脑(确保主控io0必需短接gnd后再上电,以进入刷机模式),根据自己的实际串口号设置.,点击START按钮即可开始烧录.


稍等片刻,出现![FINISH_S](https://github.com/qlwz/esp_dc1/blob/master/file/flash/FINISH_S.bmp?raw=true)即为烧录超过


注意:部分发现烧录完成后可能出现问题无法使用.可以尝试用以上烧录软件ERASE擦除一次后重新烧录.

进入烧录模式后点ERASE,显示完成即为擦除超过.再将主控板重新上电并再次进入刷机模式,重新点START烧录即可



## 致谢
以下排名不分先后，为随机。
- Huex：提供硬改方案

感谢各位使用本方法的玩家，欢迎加入QQ群776817275

## 免责申明
以上纯属个人爱好，因为使用上述方法造成的任何问题，不承担任何责任。

部分图片来源于网络，如果涉及版权，请通知删除。