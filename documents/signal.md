# E18系列模组信号测量结果

我使用频谱仪实际测量了不同的固件情况下, E18两款模组[E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123)和[E18-2G4Z27SI](http://www.ebyte.com/en/product-view-news.aspx?id=522)发射时实际功率值.

在[Koenkk zigbee2mqtt](https://github.com/Koenkk/Z-Stack-firmware/tree/master/coordinator/Z-Stack_Home_1.2)使用的修订版ZNP固件中, 默认提供的CC2530+CC2592固件, 设置TX发射功率为[TX_PWR_PLUS_19](https://github.com/Koenkk/Z-Stack-firmware/blob/master/coordinator/Z-Stack_Home_1.2/firmware.patch#L328). 但是CC2592的手册说明此PA是可以21dBm发射的.

我发现TI源码中, 设置Tx发射功率的enum数据类型最大只定义到了19dBm, 揣测可能Koenkk就是使用了默认的最大值, 而没有根据不同PA进行设置.

我修改ZNP源码并编译, 得到了不同情况下的发射信号功率图.

## 默认固件

[E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123)和[E18-2G4Z27SI](http://www.ebyte.com/en/product-view-news.aspx?id=522)情况一样. 实测发射功率为14.202dBm.
![](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/signal/14db.png)

## 修改21dBm

[E18-MS1PA1-IPX](http://www.ebyte.com/en/product-view-news.aspx?id=123)模组, 设置发射功率为2592最大功率21dBm. 代码修改如下:

```C
-      ZMacSetTransmitPower(TX_PWR_PLUS_19);
+      ZMacSetTransmitPower(21);
```

实测发射功率18.495dBm.
![](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/signal/18db.png)

## 修改27dBm

[E18-2G4Z27SI](http://www.ebyte.com/en/product-view-news.aspx?id=522)模组, 设置模组最大功率27dBm. 代码修改如下:

```C
-      ZMacSetTransmitPower(TX_PWR_PLUS_19);
+      ZMacSetTransmitPower(27);
```

实测发射功率26.809dBm.
![](https://raw.githubusercontent.com/smarthomefans/z2m_partner/master/images/signal/26db.png)

## 总结

从频谱仪的实测结果来看, 要达到最大发射功率的话, 必须要修改固件源码才行. 我并不是专业硬件人员, 对于这个结果, 我又两点疑问:

1. 最大发射功率情况下, 明显能看到谐波的毛刺, 这个毛刺会不会对数据有干扰?
2. 修改的都是最大发射功率, 但是实际使用时, zigbee传感器会向网关发数据, 这时候网关是接收状态. 我们没办法修改小米等zigbee传感器, 是不是在这个情况下, 只增大网关的发射功率, 对双向传输的zigbee协议, 是不是太大帮助?

如果有专业人士可以帮忙解答, 麻烦提issue回答一下, 先表感谢.
