# Loommii USB 键盘 → BLE 蓝牙桥接器

将 USB 有线键盘通过 ESP32-S3 转换为蓝牙（BLE）无线键盘，支持多设备切换、LED 状态指示、CapsLock 同步。

## 硬件要求

| 项目 | 规格 |
|------|------|
| 主控 | ESP32-S3 (推荐 N16R8) |
| LED | WS2812 (GPIO48) |
| USB | USB-A 母口（连接键盘） |
| 供电 | USB-C 5V |

## 烧录方法

### 方法一：Web 烧录页面（推荐）

1. 浏览器打开 [https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/](https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/)
2. 点击 "连接并烧录"
3. 选择串口 → 等待烧录完成 → 自动重启

### 方法二：esptool 命令行

```bash
python -m esptool --chip esp32s3 -b 460800 write-flash \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0x10000 ESP32S3-USB-Keyboard-To-BLE.bin
```

## 功能特性

- **蓝牙 HID 键盘** — 通过 BLE 模拟标准键盘
- **三设备切换** — 同时绑定最多 3 台主机，即时切换
- **多设备独立 MAC** — 每台设备使用独立 MAC 地址，操作系统视为不同键盘
- **绑定持久化** — 切换设备后自动重连（无需重新配对）
- **LED 状态指示** — WS2812 灯珠实时显示连接状态
- **CapsLock 同步** — 主机 CapsLock 状态同步回 USB 键盘 LED
- **解绑功能** — 快速清除与主机的绑定关系

## LED 状态说明

| 状态 | 颜色 | 说明 |
|------|------|------|
| USB 未连接 | 红色 | USB 键盘未插入或未就绪 |
| 等待配对 | 紫色 | BLE 广播中，等待主机配对 |
| 已连接 | 绿色 | 正常工作中 |
| 切换中 | 橙色 | 正在切换 BLE 连接 |
| 错误 | 红色闪烁 | USB 通信异常 |

## 快捷键操作

使用 **Scroll Lock** 作为修饰键（组合键需要先按 Scroll Lock 再按功能键）：

| 快捷键 | 功能 |
|--------|------|
| `Scroll Lock + 1` | 切换到设备 Slot 1 |
| `Scroll Lock + 2` | 切换到设备 Slot 2 |
| `Scroll Lock + 3` | 切换到设备 Slot 3 |
| `Scroll Lock + Esc` | 解绑当前设备（清除配对信息） |

### 设备名称对应

| Slot | BLE 名称 |
|------|----------|
| 1 | Loommii-KB-01 |
| 2 | Loommii-KB-02 |
| 3 | Loommii-KB-03 |

## CapsLock 同步

USB 键盘的 CapsLock LED 会自动同步主机的 CapsLock 状态。无需手动配置，即插即用。

## 首次使用流程

1. 插入 USB 键盘 → 上电 → LED 红灯亮（USB 初始化）
2. LED 变紫 → ESP32-S3 开始 BLE 广播
3. 在电脑/手机蓝牙中搜索 "Loommii-KB-01" 并配对
4. LED 变绿 → 配对成功，开始使用
5. 如需切换设备，按 `Scroll Lock + 2` 切换到第二台设备，在第二台设备搜索 "Loommii-KB-02" 配对

## 技术规格

- MCU: ESP32-S3, Xtensa LX7 双核 240MHz
- 闪存: 16MB
- PSRAM: 8MB
- BLE: Bluetooth 5.0 (Bluedroid)
- USB: USB 1.1 Host (OHCI)
- 固件: ESP-IDF v6.0.1
