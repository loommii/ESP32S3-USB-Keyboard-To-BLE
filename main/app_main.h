#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "usb/hid_host.h"

/**
 * @brief 获取当前 USB 键盘设备 handle
 *
 * 用于 bridge 层向 USB 键盘发送 SET_REPORT 请求（如 LED 状态回传）
 *
 * @return hid_host_device_handle_t  键盘 handle，未连接时返回 NULL
 */
hid_host_device_handle_t app_get_keyboard_handle(void);

#endif
