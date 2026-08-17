# BLEHomekit-Door

ESP32-C3 HomeKit BLE 门磁（纽扣电池，霍尔 / 干簧管）。

## 硬件

主控固定为 **Espressif ESP32-C3-MINI-1-N4**（4 MB Flash，板载 PCB 天线）。

完整电子 BOM、引脚、禁选料见 [`hardware/BOM.md`](hardware/BOM.md)，下单表见 [`hardware/BOM.csv`](hardware/BOM.csv)。

| 功能 | GPIO | 模组引脚 |
|------|------|----------|
| 霍尔 / 干簧管 | IO5 | 19 |
| 电池分压 ADC | IO1 | 13 |
| 恢复出厂 | IO3 | 6 |
