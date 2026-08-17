# BLEHomekit-Door 网络表

文档版本 **1.1**。主控 **ESP32-C3-MINI-1-N4**，门磁锁定 **DRV5032FBDBZR**（U3，LCSC C2655033）。

机器可读文件：

- [`netlist.csv`](netlist.csv) — 网名 / 位号 / 引脚
- [`BLEHomekit-Door.net`](BLEHomekit-Door.net) — Protel / Altium 网络表

U3 SOT-23（顶视）：**1=VCC → 3V3**，**2=OUT → HALL_OUT**，**3=GND**。磁通大于 BOP 时 OUT 为低 = 门关。SW2 干簧管本版不贴、不进网络表。

---

## 1. 器件引脚定义

### U1 ESP32-C3-MINI-1-N4

| 模组脚 | 名称 | 本板网名 |
|--------|------|----------|
| 1, 2, 11, 14, 36–53 | GND | `GND` |
| 3 | 3V3 | `3V3` |
| 4, 7, 9, 10, 15, 17, 24, 25, 28, 29, 32–35 | NC | 不连 |
| 5 | IO2 | 不连 |
| 6 | IO3 | `GPIO3` |
| 8 | EN | `CHIP_EN` |
| 12 | IO0 | 不连 |
| 13 | IO1 | `VBAT_ADC` |
| 16 | IO10 | 不连 |
| 18 | IO4 | 不连 |
| 19 | IO5 | `GPIO5` |
| 20 | IO6 | 不连 |
| 21 | IO7 | 不连 |
| 22 | IO8 | 不连 |
| 23 | IO9 | `GPIO9` |
| 26 | IO18 | 不连 |
| 27 | IO19 | 不连 |
| 30 | RXD0 / GPIO20 | `RXD0` |
| 31 | TXD0 / GPIO21 | `TXD0` |

### U2 TPS61099DRVR（WSON-6 DRV，顶视）

| 脚 | 名称 | 网名 |
|----|------|------|
| 1 | GND | `GND` |
| 2 | VOUT | `3V3` |
| 3 | FB | `FB` |
| 4 | EN | `BOOST_EN` |
| 5 | SW | `SW` |
| 6 | VIN | `VBAT` |
| 7（散热焊盘） | PAD | `GND` |

### U3 DRV5032FBDBZR（SOT-23 DBZ，顶视）

| 脚 | 名称 | 网名 |
|----|------|------|
| 1 | VCC | `3V3` |
| 2 | OUT | `HALL_OUT` |
| 3 | GND | `GND` |

### 其余

| 位号 | 脚 | 网名 |
|------|----|------|
| L1 | 1 / 2 | `VBAT` / `SW` |
| BT1 | + / − | `VBAT` / `GND` |
| SW1 | 1、2 / 3、4 | `GPIO3` / `GND`（四脚轻触，同侧内部短接） |
| J1 | 1–6 | `3V3` `GND` `TXD0` `RXD0` `GPIO9` `CHIP_EN` |
| R1 | 1 / 2 | `VBAT` / `VBAT_ADC` |
| R2 | 1 / 2 | `VBAT_ADC` / `GND` |
| R3 | 1 / 2 | `3V3` / `CHIP_EN` |
| R4 | 1 / 2 | `VBAT` / `BOOST_EN` |
| R5 | 1 / 2 | `3V3` / `GPIO9` |
| R6 | 1 / 2 | `3V3` / `GPIO3` |
| R7 | 1 / 2 | `HALL_OUT` / `GPIO5` |
| R8 | 1 / 2 | `3V3` / `FB` |
| R9 | 1 / 2 | `FB` / `GND` |
| C1、C4 | 1 / 2 | `VBAT` / `GND` |
| C2、C3、C5、C7、C8 | 1 / 2 | `3V3` / `GND` |
| C6 | 1 / 2 | `CHIP_EN` / `GND` |
| C9 | 1 / 2 | `VBAT_ADC` / `GND` |

阻容脚 1 为原理图左端，脚 2 为右端（无极性）。

---

## 2. 按网络列出的连接（网络表）

### `GND`

U1-1, U1-2, U1-11, U1-14, U1-36, U1-37, U1-38, U1-39, U1-40, U1-41, U1-42, U1-43, U1-44, U1-45, U1-46, U1-47, U1-48, U1-49, U1-50, U1-51, U1-52, U1-53  
U2-1, U2-7  
U3-3（DRV5032 GND）  
BT1-2  
C1-2, C2-2, C3-2, C4-2, C5-2, C6-2, C7-2, C8-2, C9-2  
R2-2, R9-2  
SW1-3, SW1-4  
J1-2

### `VBAT`（CR2450 正极，升压前）

BT1-1, U2-6, L1-1, R1-1, R4-1, C1-1, C4-1

### `3V3`（TPS61099 输出，模组电源）

U1-3, U2-2, U3-1（DRV5032 VCC）, C2-1, C3-1, C5-1, C7-1, C8-1, R3-1, R5-1, R6-1, R8-1, J1-1

### `SW`

U2-5, L1-2

### `FB`

U2-3, R8-2, R9-1

### `BOOST_EN`

U2-4, R4-2

### `CHIP_EN`

U1-8, R3-2, C6-1, J1-6

### `VBAT_ADC`（GPIO1，电池分压中点）

U1-13, R1-2, R2-1, C9-1

### `GPIO3`（清配对）

U1-6, R6-2, SW1-1, SW1-2

### `HALL_OUT`（DRV5032FBDBZR 脚 2 OUT）

U3-2, R7-1

### `GPIO5`（固件霍尔脚，模组脚 19）

U1-19, R7-2

### `GPIO9`（BOOT）

U1-23, R5-2, J1-5

### `TXD0`（GPIO21，模组 UART 发送）

U1-31, J1-3

### `RXD0`（GPIO20，模组 UART 接收）

U1-30, J1-4

USB-UART 交叉：J1-3（TXD0）接适配器 RX，J1-4（RXD0）接适配器 TX。

---

## 3. 关键走线（便于画原理图）

```
BT1+ ── VBAT ─┬── U2.VIN(6)
              ├── L1.1
              ├── C1, C4 → GND
              ├── R4 → BOOST_EN → U2.EN(4)
              └── R1 ── VBAT_ADC ── U1.IO1(13)
                           ├── R2 → GND
                           └── C9 → GND

L1.2 ── SW ── U2.SW(5)

U2.VOUT(2) ── 3V3 ─┬── U1.3V3(3), C2, C3, C5, C7
                   ├── U3.VCC(1), C8
                   ├── R3 → CHIP_EN → U1.EN(8), C6, J1-6
                   ├── R5 → GPIO9 → U1.IO9(23), J1-5
                   ├── R6 → GPIO3 → U1.IO3(6), SW1
                   └── R8 → FB → U2.FB(3), R9 → GND

U3.VCC(1) ── 3V3
U3.OUT(2) ── HALL_OUT ── R7(0Ω) ── GPIO5 ── U1.IO5(19)
U3.GND(3) ── GND

U1.TXD0(31) ── J1-3
U1.RXD0(30) ── J1-4
```

磁铁靠近 DRV5032FBDBZR → OUT 拉低 → GPIO5=0 → 门关。
