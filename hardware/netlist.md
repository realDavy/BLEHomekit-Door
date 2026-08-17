# BLEHomekit-Door 网络表

文档版本 **1.3**。主控 **ESP32-C3-MINI-1-N4**，门磁锁定 **TMAG5233D1EDBVR**（侧感应，U3，LCSC **C44538511**，立创现货）+ **2N7002** 反相。

机器可读文件：

- [`netlist.csv`](netlist.csv)
- [`BLEHomekit-Door.net`](BLEHomekit-Door.net)

U3 SOT-23：**1=VCC → 3V3**，**2=OUT → HALL_OUT**，**3=GND**。感应轴平行于 PCB（pin1↔pin2，侧向来磁）。

极性（门关常态不耗上拉电流）：

- 磁铁侧向靠近（门关）→ U3.OUT 低 → Q1 关 → GPIO5 被 R10 拉高 → 门关
- 磁铁离开（门开）→ U3.OUT 高 → Q1 开 → GPIO5 拉低 → 门开

---

## 1. 器件引脚定义

### U1 ESP32-C3-MINI-1-N4

| 模组脚 | 名称 | 本板网名 |
|--------|------|----------|
| 1, 2, 11, 14, 36–53 | GND | `GND` |
| 3 | 3V3 | `3V3` |
| 4, 7, 9, 10, 15, 17, 24, 25, 28, 29, 32–35 | NC | 不连 |
| 6 | IO3 | `GPIO3` |
| 8 | EN | `CHIP_EN` |
| 13 | IO1 | `VBAT_ADC` |
| 19 | IO5 | `GPIO5` |
| 23 | IO9 | `GPIO9` |
| 30 | RXD0 / GPIO20 | `RXD0` |
| 31 | TXD0 / GPIO21 | `TXD0` |

未列出的 GPIO 不连。

### U2 TPS61099DRVR（WSON-6 DRV）

| 脚 | 名称 | 网名 |
|----|------|------|
| 1 | GND | `GND` |
| 2 | VOUT | `3V3` |
| 3 | FB | `FB` |
| 4 | EN | `BOOST_EN` |
| 5 | SW | `SW` |
| 6 | VIN | `VBAT` |
| 7（焊盘） | PAD | `GND` |

### U3 TMAG5233D1EDBVR（SOT-23 DBV，顶视）

| 脚 | 名称 | 网名 |
|----|------|------|
| 1 | VCC | `3V3` |
| 2 | OUT | `HALL_OUT` |
| 3 | GND | `GND` |

### Q1 2N7002（SOT-23）

| 脚 | 名称 | 网名 |
|----|------|------|
| 1 | G | `HALL_OUT` |
| 2 | S | `GND` |
| 3 | D | `GPIO5` |

### 其余

| 位号 | 脚 | 网名 |
|------|----|------|
| L1 | 1 / 2 | `VBAT` / `SW` |
| BT1 | + / − | `VBAT` / `GND` |
| SW1 | 1、2 / 3、4 | `GPIO3` / `GND` |
| J1 | 1–6 | `3V3` `GND` `TXD0` `RXD0` `GPIO9` `CHIP_EN` |
| R1 | 1 / 2 | `VBAT` / `VBAT_ADC` |
| R2 | 1 / 2 | `VBAT_ADC` / `GND` |
| R3 | 1 / 2 | `3V3` / `CHIP_EN` |
| R4 | 1 / 2 | `VBAT` / `BOOST_EN` |
| R5 | 1 / 2 | `3V3` / `GPIO9` |
| R6 | 1 / 2 | `3V3` / `GPIO3` |
| R8 | 1 / 2 | `3V3` / `FB` |
| R9 | 1 / 2 | `FB` / `GND` |
| R10 | 1 / 2 | `3V3` / `GPIO5` |
| C1、C4 | 1 / 2 | `VBAT` / `GND` |
| C2、C3、C5、C7、C8 | 1 / 2 | `3V3` / `GND` |
| C6 | 1 / 2 | `CHIP_EN` / `GND` |
| C9 | 1 / 2 | `VBAT_ADC` / `GND` |

---

## 2. 按网络列出的连接

### `GND`

U1-1, U1-2, U1-11, U1-14, U1-36 … U1-53  
U2-1, U2-7  
U3-3  
Q1-2  
BT1-2  
C1-2, C2-2, C3-2, C4-2, C5-2, C6-2, C7-2, C8-2, C9-2  
R2-2, R9-2  
SW1-3, SW1-4  
J1-2

### `VBAT`

BT1-1, U2-6, L1-1, R1-1, R4-1, C1-1, C4-1

### `3V3`

U1-3, U2-2, U3-1, C2-1, C3-1, C5-1, C7-1, C8-1, R3-1, R5-1, R6-1, R8-1, R10-1, J1-1

### `SW`

U2-5, L1-2

### `FB`

U2-3, R8-2, R9-1

### `BOOST_EN`

U2-4, R4-2

### `CHIP_EN`

U1-8, R3-2, C6-1, J1-6

### `VBAT_ADC`

U1-13, R1-2, R2-1, C9-1

### `GPIO3`

U1-6, R6-2, SW1-1, SW1-2

### `HALL_OUT`

U3-2, Q1-1

### `GPIO5`（高=门关，低=门开）

U1-19, Q1-3, R10-2

### `GPIO9`

U1-23, R5-2, J1-5

### `TXD0`

U1-31, J1-3

### `RXD0`

U1-30, J1-4

---

## 3. 关键走线

```
U3.VCC(1) ── 3V3，C8
U3.OUT(2) ── HALL_OUT ── Q1.G(1)
U3.GND(3) ── GND

Q1.S(2) ── GND
Q1.D(3) ── GPIO5 ── U1.IO5(19)
                 └── R10 ── 3V3

磁铁从 PCB 侧面沿 pin1–pin2 轴向靠近 U3。
```
