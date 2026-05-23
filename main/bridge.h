#ifndef BRIDGE_H
#define BRIDGE_H

#include "esp_err.h"
#include "hid_types.h"

#define MAX_DEVICE_SLOTS 3

/* 初始化桥接任务和队列 */
esp_err_t bridge_init(void);

/* USB HOST 回调：将键盘报告送入桥接队列 */
void bridge_handle_keyboard_report(const uint8_t *data, size_t length);

/* 处理 BLE 主机发来的 LED 报告（CapsLock 等状态） */
void bridge_handle_led_report(uint8_t led_bitmap);

/* 获取当前设备槽位 */
uint8_t bridge_get_current_slot(void);

/* BLE 连接/断开通知，更新内部状态 */
void bridge_on_ble_connected(void);
void bridge_on_ble_disconnected(void);

/* 请求清除当前连接设备的配对信息（蓝牙解绑） */
void bridge_request_unpair(void);

/* 配对开始/结束回调：切换配对模式，初始化 FIFO，控制 LED */
void bridge_on_pairing_start(void);
void bridge_on_pairing_done(bool success);

#endif
