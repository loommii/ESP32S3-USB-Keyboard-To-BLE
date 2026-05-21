#ifndef BRIDGE_H
#define BRIDGE_H

#include "esp_err.h"
#include "hid_types.h"

esp_err_t bridge_init(void);

void bridge_handle_keyboard_report(const uint8_t *data, size_t length);

#endif
