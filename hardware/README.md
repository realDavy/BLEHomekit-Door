# 硬件

HomeKit BLE 门磁。主控 **ESP32-C3-MINI-1-N4**，侧感应霍尔 **TMAG5233D1BDBVR** + **2N7002** 反相。

- 电子料单：[`BOM.md`](BOM.md) / [`BOM.csv`](BOM.csv)
- 网络表：[`netlist.md`](netlist.md) / [`netlist.csv`](netlist.csv) / [`BLEHomekit-Door.net`](BLEHomekit-Door.net)

GPIO5：**高 = 门关（常态）**，**低 = 门开**。磁铁从 PCB 侧面靠近。
