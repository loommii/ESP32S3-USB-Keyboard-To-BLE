#ifndef USB_HOST_MANAGER_H
#define USB_HOST_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t usb_host_manager_init(void);

esp_err_t usb_host_manager_deinit(void);

TaskHandle_t usb_host_manager_get_task_handle(void);

#endif
