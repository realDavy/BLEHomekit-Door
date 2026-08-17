# ESP32-C3 HomeKit BLE 门磁

把 [BLEHomeKit](https://github.com/realDavy/BLEHomeKit) 的 HAP-BLE 协议栈改成 **Apple HomeKit 门配件**：ESP32-C3 + 侧感应霍尔，**没有屏幕、旋钮和 LED**，纽扣电池供电。家庭 App 里可以看到 **开关状态** 和 **电池电量**。

本配件只走 **蓝牙 LE（HAP-BLE）**，不连 Wi-Fi。

主控 **ESP32-C3-MINI-1-N4**。电子 BOM：[`hardware/BOM.md`](hardware/BOM.md)，网络表：[`hardware/netlist.md`](hardware/netlist.md)。

## 硬件

| 功能 | GPIO | 说明 |
|------|------|------|
| 门磁（反相后） | **IO5** | 模组脚 19。**高 = 门关，低 = 门开**。IO0–5 才能深睡唤醒 |
| 电池分压 ADC | **IO1** | `VBAT — 1MΩ — IO1 — 1MΩ — GND`（2:1）。不接则电量显示 100% |
| 恢复出厂（配对） | **IO3** | 上电时对地保持约 1.5 秒，清除 HomeKit 配对 |

ESP32-C3 深睡 GPIO 唤醒只有 **IO0–IO5**，门磁接 **IO5**。不要用 IO11–IO17（模组内部 SPI Flash，IO14 是片选，当 GPIO 会看门狗复位）。

### 传感器（侧感应 + 门开拉低）

门大部分时间是关的。若「磁铁靠近 → GPIO 拉低」当门关，内部上拉会一直灌电流。本板改为：

1. 霍尔：**TI TMAG5233D1BDBVR**，**平面内 / 侧感应**（X 轴，磁场平行于 PCB），全极推挽，5 Hz，Iq ≈ 0.55 µA。不要用正面感应的 DRV5032FBDBZR，也不要用 A3144。
2. NMOS 反相（2N7002）：霍尔 OUT → 栅极；漏极 → GPIO5；1 MΩ 上拉到 3V3。
3. 固件 `BOARD_HALL_CLOSED_LEVEL = 1`（GPIO 高 = 门关）。

逻辑：

- 磁铁靠近（门关，常态）→ 霍尔 OUT 低 → MOS 关 → GPIO5 **高** → HomeKit 关闭
- 磁铁离开（门开）→ 霍尔 OUT 高 → MOS 开 → GPIO5 **低** → HomeKit 打开

磁铁从 PCB **侧面**靠近封装的 pin1–pin2 轴向（平行于板面），不要从芯片顶面正对。

### 电池电量

家庭 App 配件详情里会显示 **电池百分比**，并带低电量状态（默认 ≤20%）。

电量按 CR2032/CR2450 曲线估算：3.0 V = 100%，2.0 V = 0%。分压比、满/空电压可在 `board_pins.hpp` 修改。

ESP32-C3 工作电压约 3.0–3.6 V，CR2032 末期会偏低。量产建议用 **CR2450**，或给模组加升压。板上若有常亮电源 LED，请去掉，否则纽扣电池撑不久。

## 功耗策略

- 未配对：一直 BLE 广播（20 ms），方便手机添加
- 已配对、空闲约 **12 秒** 且没有蓝牙连接：关射频，进入深睡（IO5 电平变化 + 30 分钟定时唤醒）
- 门状态变化：立即醒来，快广播 3 秒（20 ms），再以 1280 ms 间隔广播，等家庭中枢连上并同步
- CPU 80 MHz、NimBLE modem sleep、tickless idle、关 Wi-Fi
- 广播/连接发射功率 0 dBm

深睡电流主要取决于模组 LDO 和霍尔。裸片 ESP32-C3 深睡约十几 µA；开发板 LDO 可能到数百 µA。门关时 GPIO5 为高，上拉不耗电。

## 软件要求

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/index.html) **v5.3 或更高**
- Python 3 + ESP-IDF 组件管理器

## 构建与烧录

```bash
git clone --recurse-submodules https://github.com/realDavy/BLEHomekit-Door.git
cd BLEHomekit-Door

. $IDF_PATH/export.sh
rm -f sdkconfig
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

没有屏幕，**配对码只在串口打印**。首次上电请看日志里的 `Setup code: XXX-XX-XXX`。

## HomeKit 配对

配对码由出厂 MAC 生成（`XXX-XX-XXX`），同一块板不变。Apple 禁止的弱码会被自动跳过。

1. USB 看串口，记下 PIN
2. iPhone「家庭」→ 添加配件 →「更多选项…」
3. 选择 **Door**（蓝牙配件）
4. 输入配对码

Identify 时本硬件没有灯，只会打日志。

清除配对：

- 上电时把 **IO3 拉低约 1.5 秒**，或
- 开机 8 秒内把门开关 **10 次**，或
- `idf.py erase-nvs`

然后先在 iPhone「家庭」里删掉旧配件再添加。

BLE-only 配件在没人连着时，家庭主页常显示「未响应」：

- 添加完成后先退出设置页
- iPhone 开着 Wi-Fi 时，家庭会把控制交给中枢。中枢不在板子旁边 → 未响应
- 没有 HomePod / Apple TV 时，点进配件、手机用蓝牙连上才会变成可控制

配对成功后会深睡。开门/关门应在几秒内同步到家庭 App。

## HomeKit 服务

| 服务 | 特征 | 含义 |
|------|------|------|
| Contact Sensor（主服务） | ContactSensorState | 只检测开/关，没有电机、没有开度滑条 |
| Battery | BatteryLevel、StatusLowBattery、ChargingState | 电量 0–100%，纽扣电池为不可充电 |

配件类别为 **Sensor**（门磁/接触传感器）。厂商名 **Aidaegis**。

若家庭 App 里还显示开度百分比滑条，是旧固件的 Door 服务。请烧录本版本后，在「家庭」里删掉配件再添加。

### 门状态不对时看 pin scan

固件每 2 秒打印 `pin scan IO0=… IO4=… IO5=…`。门磁在 **IO5**（反相后：**1 = 门关，0 = 门开**）。

1. 门关（磁铁侧向靠近）应出现 `GPIO5 HIGH → door CLOSED`，且 **IO5=1**
2. 门开（磁铁离开）应为 `GPIO5 LOW → door OPEN`，IO5=0
3. 调试可把 GPIO5 接到 3V3 模拟门关，接到 GND 模拟门开
4. 若 IO5 仍是 1、IO4 变成 0：碰到的是 IO4，不是 IO5
5. 串口已 CLOSED、家庭 App 仍开着：删掉配件再添加，iPhone 靠近用蓝牙打开详情

已配对且没有蓝牙连接时，约 12 秒会深睡，USB 串口会断开（`COM 连接意外断开`），这是正常的。调试门磁时保持家庭 App 开着配件详情（有 BLE 连接就不会睡）。

## 工程结构

```
├── main/                      # 门磁、电池 ADC、深睡、HAP 配件
├── hardware/                  # 电子 BOM、网络表
├── components/hap_wrapper/    # 封装 realDavy/HAP（git submodule）
├── components/yahap-pal/      # ESP-IDF 平台层（NimBLE / NVS / 加密）
├── partitions.csv
└── sdkconfig.defaults         # ESP32-C3 + NimBLE + 低功耗
```

HAP 协议栈来自 [realDavy/HAP](https://github.com/realDavy/HAP)，平台层来自 [esp32-yahap-pal](https://github.com/rednblkx/esp32-yahap-pal)。
