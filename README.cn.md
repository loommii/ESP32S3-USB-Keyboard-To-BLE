[🌏 中文](README.cn.md) | [English](README.md)

---

# USB 键盘 → BLE 蓝牙桥接器

**版本: V1.0.0**

<p align="center"><img src="logo.png" width="260" alt="logo"></p>

将 USB 有线键盘通过 ESP32-S3 转换为蓝牙（BLE）无线键盘，支持多设备切换

## 兼容性测试

> 以下为作者实际测试环境，未列出的设备和系统不代表不兼容。

| 键盘 | 主机 | 结果 |
|------|------|------|
| 罗技 K845 | Windows PC（Intel AX201 蓝牙） | ✓ |
| 罗技 K845 | 安卓平板 | ✓ |
| 罗技 K845 | Mac Mini M4 | ✓ |
| — | Web 烧录页面 | ✓ |

## 硬件要求

| 项目 | 规格 |
|------|------|
| 主控 | ESP32-S3 系列（作者使用 ESP32-S3-N16R8，网购约 ¥20） |


## 烧录方法

### 方法一：Web 烧录页面（推荐）

1. 浏览器打开 [https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/](https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/)
2. 点击 "连接并烧录"
3. 选择串口 → 等待烧录完成 → 自动重启


## 功能特性

- **蓝牙 HID 键盘** — 通过 BLE 模拟标准键盘
- **三设备切换** — 同时绑定最多 3 台主机，即时切换
- **多设备独立 MAC** — 每台设备使用独立 MAC 地址，操作系统视为不同键盘
- **绑定持久化** — 切换设备后自动重连（无需重新配对）
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

## 首次使用流程

1. 插入 USB 键盘 → 上电 → LED 红灯亮（USB 初始化）
2. LED 变紫 → ESP32-S3 开始 BLE 广播
3. 在电脑/手机蓝牙中搜索 "Loommii-KB-01" 并配对
4. LED 变绿 → 配对成功，开始使用
5. 如需切换设备，按 `Scroll Lock + 2` 切换到第二台设备，在第二台设备搜索 "Loommii-KB-02" 配对

## 技术规格
- ESP-IDF v6.0.1
