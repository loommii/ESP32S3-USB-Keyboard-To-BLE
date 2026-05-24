[🌏 中文](README.cn.md) | [English](README.md)

---

# USB 键盘 → BLE 蓝牙桥接器

**版本: V1.1.0**

> **V1.1.0** — BLE 协议栈重构（Bluedroid → NimBLE），升级安全配对为 Passkey Entry（6 位验证码，MITM 保护）。
> ⚠ 如遇到兼容性问题，请改用 V1.0.1 版本。
>
> **V1.0.1** — 用官方 USB/BLE 驱动库替换自定义代码；新增 Web 烧录页面。
>
> **V1.0.0** — 初始版本。USB 键盘转 BLE 桥接（Bluedroid），CapsLock/NumLock LED 同步，3 设备切换（独立 MAC），快捷键切换/解绑，LED 状态指示。

<p align="center"><img src="logo.png" width="260" alt="logo"></p>

将 USB 有线键盘通过 ESP32-S3 转换为蓝牙（BLE）无线键盘，支持最多 3 台设备切换。

## 首次使用流程

1. 插入 USB 键盘 → 上电 → **红灯**亮（USB 初始化）
2. 灯变 **紫色闪烁** → 板子开始 BLE 广播
3. 在电脑/手机蓝牙中搜索 **"Loommii-KB-01"** 并配对
4. 主机屏幕会显示一个 **6 位配对码**
5. **在 USB 键盘上输入该配对码**，然后按 **Enter** 提交
6. 灯变 **绿色** → 配对成功，开始使用
7. 如需切换设备：按 `Scroll Lock + 2`（或 `Scroll Lock + 3`）切换到第二/三台设备，在该设备上搜索 **"Loommii-KB-02"**（或 **"-03"**）并重复第 3–6 步

> 配对码输入过程中随时按 **Esc** 可取消配对。

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
2. 点击"连接并烧录"
3. 选择串口 → 等待烧录完成 → 自动重启


## 功能特性

- **有线键盘转 BLE 无线** — USB 键盘即插即用，蓝牙连接电脑/平板/手机
- **三设备切换** — 同时绑定最多 3 台主机，按快捷键即时切换
- **独立 MAC 地址** — 每台设备使用独立 MAC 地址，操作系统视为不同物理键盘
- **绑定持久化** — 切换后自动重连已配对设备，无需重新配对
- **CapsLock / NumLock / ScrollLock 灯同步** — 蓝牙主机关闭 NumLock，键盘指示灯同步熄灭
- **键盘键入配对码** — 配对码直接在 USB 键盘上输入，无需屏幕或其他按键
- **快捷键切换/解绑** — Scroll Lock 修饰键组合操作，无需额外按键

## LED 状态说明

| 状态 | 颜色 | 说明 |
|------|------|------|
| USB 未连接 | 红色常亮 | USB 键盘未插入或未就绪 |
| 等待蓝牙 | 紫色闪烁 | USB 已就绪，BLE 广播中，等待主机连接 |
| 已连接 | 绿色常亮 | USB + BLE 均已连接，正常工作 |
| 配对码输入中 | 紫色常亮 | 正在等待用户在键盘上输入配对码并按 Enter |
| 切换中 | 橙色常亮 | 正在切换设备（即将重启） |
| 错误 | 红色常亮 | USB 通信异常 |

## 快捷键操作

**Scroll Lock** 作为修饰键（先按 Scroll Lock 再按功能键）：

| 快捷键 | 功能 |
|--------|------|
| `Scroll Lock + 1` | 切换到设备 Slot 1 |
| `Scroll Lock + 2` | 切换到设备 Slot 2 |
| `Scroll Lock + 3` | 切换到设备 Slot 3 |
| `Scroll Lock + Esc` | 解绑当前设备（清除配对信息） |

## 设备名称对应

| Slot | BLE 名称 |
|------|----------|
| 1 | Loommii-KB-01 |
| 2 | Loommii-KB-02 |
| 3 | Loommii-KB-03 |

## 技术规格

| 项目 | 说明 |
|------|------|
| SDK | ESP-IDF v6.0.1 |
| BLE 协议栈 | Apache NimBLE |
| 蓝牙版本 | BLE 5.0（兼容 BLE 4.x 以上设备） |
| 供电方式 | 通过 ESP32-S3 开发板的 USB 口供电（无需额外电源） |
| 指示灯 | 板载 RGB LED，颜色指示当前工作状态 |

## 开源许可

Copyright (C) 2026 loommii

本项目基于 **GNU Affero General Public License v3.0**（AGPL-3.0）发布。
完整协议见 [LICENSE](LICENSE) 文件。
