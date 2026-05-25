[English](README.md) | [🌏 中文](README.cn.md)

---

# USB Keyboard → BLE Bridge

**Version: V1.1.1**

> **V1.1.1** — Optimized BLE initialization stability — increased main task stack size for reliable startup.
>
> **V1.1.0** — BLE stack rebuilt (Bluedroid → NimBLE), security upgraded to Passkey Entry (6-digit verification code, MITM protection).
> ⚠ If you encounter compatibility issues, please use V1.0.1 instead.
>
> **V1.0.1** — Replaced custom USB/BLE drivers with official ESP-IDF components; added Web flashing page.
>
> **V1.0.0** — Initial release. USB keyboard to BLE bridge (Bluedroid), CapsLock/NumLock LED sync, 3-slot device switching (independent MAC), hotkey switching/unpairing, LED status indicator.

<p align="center"><img src="logo.png" width="260" alt="logo"></p>

Turns a wired USB keyboard into a wireless Bluetooth (BLE) keyboard using an ESP32-S3, with support for up to 3 paired hosts.

## First-Time Setup

1. Plug in the USB keyboard → power on → **red LED** (USB initializing)
2. LED changes to **blinking purple** → board starts BLE advertising
3. Search for **"Loommii-KB-01"** on your computer or phone and pair
4. A **6-digit pairing code** appears on the host screen
5. **Type the pairing code on your USB keyboard** and press **Enter**
6. LED turns **green** → pairing successful, ready to use
7. To switch devices: press `Scroll Lock + 2` (or `Scroll Lock + 3`) to switch to the second/third device, search for **"Loommii-KB-02"** (or **"-03"**) on that device and repeat steps 3–6

> Press **Esc** at any time during pairing code entry to cancel pairing.

## Compatibility

> Tested by the author. Devices and systems not listed may still be compatible.

| Keyboard | Host | Result |
|----------|------|--------|
| Logitech K845 | Windows PC (Intel AX201 Bluetooth) | ✓ |
| Logitech K845 | Android Tablet | ✓ |
| Logitech K845 | Mac Mini M4 | ✓ |
| — | Web flashing page | ✓ |

## Hardware Requirements

| Item | Spec |
|------|------|
| MCU | ESP32-S3 series (author uses ESP32-S3-N16R8, approx. ¥20 online) |

## Flashing

### Method 1: Web flashing page (Recommended)

1. Open [https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/](https://loommii.github.io/ESP32S3-USB-Keyboard-To-BLE/)
2. Click "Connect and flash"
3. Select the serial port → wait for completion → device reboots automatically


## Features

- **Wired keyboard to BLE wireless** — plug-and-play USB keyboard, Bluetooth connectivity to PC/tablet/phone
- **3-host switching** — pair with up to 3 hosts, switch instantly via hotkey
- **Independent MAC addresses** — each slot uses its own MAC, hosts see them as distinct physical keyboards
- **Persistent bonding** — auto-reconnects on switch, no re-pairing needed
- **CapsLock / NumLock / ScrollLock LED sync** — turn off NumLock on the host, the keyboard LED goes out simultaneously
- **Keyboard pairing code entry** — type the pairing code directly on your USB keyboard, no screen or extra buttons needed
- **Hotkey switching & unpairing** — Scroll Lock modifier combos, no extra buttons

## LED Status

| State | Color | Description |
|-------|-------|-------------|
| USB disconnected | Solid red | USB keyboard not plugged in or not ready |
| Waiting for Bluetooth | Blinking purple | USB ready, BLE advertising, waiting for host |
| Connected | Solid green | USB + BLE both connected, normal operation |
| Pairing code entry | Solid purple | Waiting for user to type the pairing code on the keyboard and press Enter |
| Switching | Solid orange | Device slot switching in progress (reboot pending) |
| Error | Solid red | USB communication error |

## Hotkeys

**Scroll Lock** is used as the modifier (press Scroll Lock first, then the action key):

| Hotkey | Action |
|--------|--------|
| `Scroll Lock + 1` | Switch to Slot 1 |
| `Scroll Lock + 2` | Switch to Slot 2 |
| `Scroll Lock + 3` | Switch to Slot 3 |
| `Scroll Lock + Esc` | Unpair current device (clear bonding) |

## Device Names

| Slot | BLE Name |
|------|----------|
| 1 | Loommii-KB-01 |
| 2 | Loommii-KB-02 |
| 3 | Loommii-KB-03 |

## Specifications

| Item | Details |
|------|---------|
| SDK | ESP-IDF v6.0.1 |
| BLE Stack | Apache NimBLE |
| Bluetooth Version | BLE 5.0 (compatible with BLE 4.x and later) |
| Power | Powered via ESP32-S3 USB port (no external supply needed) |
| Indicator | On-board RGB LED, color indicates device state |

## License

Copyright (C) 2026 loommii

This project is licensed under the **GNU Affero General Public License v3.0** (AGPL-3.0).
See the [LICENSE](LICENSE) file for the full license text.
