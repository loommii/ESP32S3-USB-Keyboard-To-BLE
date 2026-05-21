[中文](README.cn.md) | [🌏 English](README.md)

---

# USB Keyboard → BLE Bridge

**Version: V1.0.0**

<p align="center"><img src="logo.png" width="80%" alt="logo" style="max-width: 278px;"></p>

Convert a wired USB keyboard to a wireless BLE keyboard via ESP32-S3. Supports multi-device switching.

## Compatibility Test

> These are the author's test results. Unlisted devices/systems are not necessarily incompatible.

| Keyboard | Host | Result |
|----------|------|--------|
| Logitech K845 | Windows PC (Intel AX201 Bluetooth) | ✓ |
| Logitech K845 | Android Tablet | ✓ |
| Logitech K845 | Mac Mini M4 | ✓ |
| — | Web Flasher | ✓ |

## Hardware Requirements

| Item | Spec |
|------|------|
| MCU | ESP32-S3 series (author uses ESP32-S3-N16R8, ~¥20 online) |

## Flashing

### Method 1: Web Flasher (Recommended)

1. Open [https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/](https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/)
2. Click "Connect & Flash"
3. Select serial port → wait for completion → auto reboot

## Features

- **BLE HID Keyboard** — Emulates a standard keyboard via BLE
- **3-Device Switching** — Pair with up to 3 hosts, switch instantly
- **Independent MAC** — Each slot uses a unique MAC address; OS treats them as separate keyboards
- **Persistent Bonding** — Auto-reconnects when switching slots (no re-pairing needed)
- **Unpair** — Quickly clear pairing with the current host

## LED Status

| State | Color | Description |
|-------|-------|-------------|
| USB Not Connected | Red | USB keyboard not plugged in or not ready |
| Waiting for Pairing | Purple | BLE advertising, waiting for host to pair |
| Connected | Green | Normal operation |
| Switching | Orange | Switching BLE connection |
| Error | Red flash | USB communication error |

## Shortcuts

Use **Scroll Lock** as the modifier key (press Scroll Lock first, then the function key):

| Shortcut | Action |
|----------|--------|
| `Scroll Lock + 1` | Switch to Slot 1 |
| `Scroll Lock + 2` | Switch to Slot 2 |
| `Scroll Lock + 3` | Switch to Slot 3 |
| `Scroll Lock + Esc` | Unpair current device |

### Device Names

| Slot | BLE Name |
|------|----------|
| 1 | Loommii-KB-01 |
| 2 | Loommii-KB-02 |
| 3 | Loommii-KB-03 |

## First-Time Use

1. Plug in USB keyboard → power on → LED red (USB init)
2. LED turns purple → BLE advertising
3. Search "Loommii-KB-01" on your PC/tablet/phone and pair
4. LED turns green → ready to use
5. To switch devices, press `Scroll Lock + 2`, then pair on second device as "Loommii-KB-02"

## Tech Specs

- ESP-IDF v6.0.1
