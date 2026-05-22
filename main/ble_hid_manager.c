#include "ble_hid_manager.h"
#include "ble_hid_report.h"
#include "bridge.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_hidd.h"
#include "esp_hidd_gatts.h"
#include "esp_hid_common.h"
#include "nvs_flash.h"
#include "config.h"
#include "string.h"

static const char *TAG = "BLE_HID";

static bool s_sec_conn = false;
static esp_bd_addr_t s_remote_bda = {0};
static esp_hidd_dev_t *s_hid_dev = NULL;  /* esp_hid 组件设备句柄，用于发送输入报告和查询连接状态 */

static const char *s_device_name = DEVICE_NAME;
static char s_device_name_buf[32] = DEVICE_NAME;

static uint8_t hidd_service_uuid128[] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x03C1,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x30,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static const uint8_t hid_report_map[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x02,  // Usage (Mouse)
    0xA1, 0x01,  // Collection (Application)
    0x85, 0x01,  // Report Id (1)
    0x09, 0x01,  //   Usage (Pointer)
    0xA1, 0x00,  //   Collection (Physical)
    0x05, 0x09,  //     Usage Page (Buttons)
    0x19, 0x01,  //     Usage Minimum (01) - Button 1
    0x29, 0x03,  //     Usage Maximum (03) - Button 3
    0x15, 0x00,  //     Logical Minimum (0)
    0x25, 0x01,  //     Logical Maximum (1)
    0x75, 0x01,  //     Report Size (1)
    0x95, 0x03,  //     Report Count (3)
    0x81, 0x02,  //     Input (Data, Variable, Absolute) - Button states
    0x75, 0x05,  //     Report Size (5)
    0x95, 0x01,  //     Report Count (1)
    0x81, 0x01,  //     Input (Constant) - Padding or Reserved bits
    0x05, 0x01,  //     Usage Page (Generic Desktop)
    0x09, 0x30,  //     Usage (X)
    0x09, 0x31,  //     Usage (Y)
    0x09, 0x38,  //     Usage (Wheel)
    0x15, 0x81,  //     Logical Minimum (-127)
    0x25, 0x7F,  //     Logical Maximum (127)
    0x75, 0x08,  //     Report Size (8)
    0x95, 0x03,  //     Report Count (3)
    0x81, 0x06,  //     Input (Data, Variable, Relative) - X & Y coordinate
    0xC0,        //   End Collection
    0xC0,        // End Collection

    0x05, 0x01,  // Usage Pg (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection: (Application)
    0x85, 0x02,  // Report Id (2)
    0x05, 0x07,  //   Usage Pg (Key Codes)
    0x19, 0xE0,  //   Usage Min (224)
    0x29, 0xE7,  //   Usage Max (231)
    0x15, 0x00,  //   Log Min (0)
    0x25, 0x01,  //   Log Max (1)

    0x75, 0x01,  //   Report Size (1)
    0x95, 0x08,  //   Report Count (8)
    0x81, 0x02,  //   Input: (Data, Variable, Absolute)

    0x95, 0x01,  //   Report Count (1)
    0x75, 0x08,  //   Report Size (8)
    0x81, 0x01,  //   Input: (Constant)

    0x05, 0x08,  //   Usage Pg (LEDs)
    0x19, 0x01,  //   Usage Min (1)
    0x29, 0x05,  //   Usage Max (5)
    0x95, 0x05,  //   Report Count (5)
    0x75, 0x01,  //   Report Size (1)
    0x91, 0x02,  //   Output: (Data, Variable, Absolute)

    0x95, 0x01,  //   Report Count (1)
    0x75, 0x03,  //   Report Size (3)
    0x91, 0x01,  //   Output: (Constant)

    0x95, 0x06,  //   Report Count (6)
    0x75, 0x08,  //   Report Size (8)
    0x15, 0x00,  //   Log Min (0)
    0x25, 0x65,  //   Log Max (101)
    0x05, 0x07,  //   Usage Pg (Key Codes)
    0x19, 0x00,  //   Usage Min (0)
    0x29, 0x65,  //   Usage Max (101)
    0x81, 0x00,  //   Input: (Data, Array)

    0xC0,        // End Collection

    0x05, 0x0C,   // Usage Pg (Consumer Devices)
    0x09, 0x01,   // Usage (Consumer Control)
    0xA1, 0x01,   // Collection (Application)
    0x85, 0x03,   // Report Id (3)
    0x09, 0x02,   //   Usage (Numeric Key Pad)
    0xA1, 0x02,   //   Collection (Logical)
    0x05, 0x09,   //     Usage Pg (Button)
    0x19, 0x01,   //     Usage Min (Button 1)
    0x29, 0x0A,   //     Usage Max (Button 10)
    0x15, 0x01,   //     Logical Min (1)
    0x25, 0x0A,   //     Logical Max (10)
    0x75, 0x04,   //     Report Size (4)
    0x95, 0x01,   //     Report Count (1)
    0x81, 0x00,   //     Input (Data, Ary, Abs)
    0xC0,         //   End Collection
    0x05, 0x0C,   //   Usage Pg (Consumer Devices)
    0x09, 0x86,   //   Usage (Channel)
    0x15, 0xFF,   //   Logical Min (-1)
    0x25, 0x01,   //   Logical Max (1)
    0x75, 0x02,   //   Report Size (2)
    0x95, 0x01,   //   Report Count (1)
    0x81, 0x46,   //   Input (Data, Var, Rel, Null)
    0x09, 0xE9,   //   Usage (Volume Up)
    0x09, 0xEA,   //   Usage (Volume Down)
    0x15, 0x00,   //   Logical Min (0)
    0x75, 0x01,   //   Report Size (1)
    0x95, 0x02,   //   Report Count (2)
    0x81, 0x02,   //   Input (Data, Var, Abs)
    0x09, 0xE2,   //   Usage (Mute)
    0x09, 0x30,   //   Usage (Power)
    0x09, 0x83,   //   Usage (Recall Last)
    0x09, 0x81,   //   Usage (Assign Selection)
    0x09, 0xB0,   //   Usage (Play)
    0x09, 0xB1,   //   Usage (Pause)
    0x09, 0xB2,   //   Usage (Record)
    0x09, 0xB3,   //   Usage (Fast Forward)
    0x09, 0xB4,   //   Usage (Rewind)
    0x09, 0xB5,   //   Usage (Scan Next)
    0x09, 0xB6,   //   Usage (Scan Prev)
    0x09, 0xB7,   //   Usage (Stop)
    0x15, 0x01,   //   Logical Min (1)
    0x25, 0x0C,   //   Logical Max (12)
    0x75, 0x04,   //   Report Size (4)
    0x95, 0x01,   //   Report Count (1)
    0x81, 0x00,   //   Input (Data, Ary, Abs)
    0x09, 0x80,   //   Usage (Selection)
    0xA1, 0x02,   //   Collection (Logical)
    0x05, 0x09,   //     Usage Pg (Button)
    0x19, 0x01,   //     Usage Min (Button 1)
    0x29, 0x03,   //     Usage Max (Button 3)
    0x15, 0x01,   //     Logical Min (1)
    0x25, 0x03,   //     Logical Max (3)
    0x75, 0x02,   //     Report Size (2)
    0x81, 0x00,   //     Input (Data, Ary, Abs)
    0xC0,           //   End Collection
    0x81, 0x03,   //   Input (Const, Var, Abs)
    0xC0,            // End Collection
};

static esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = hid_report_map,
        .len = sizeof(hid_report_map),
    },
};

/* BLE HID 设备配置：VID/PID/版本/设备名/制造商/序列号/报告映射表 */
static esp_hid_device_config_t ble_hid_config = {
    .vendor_id          = BLE_VENDOR_ID,
    .product_id         = BLE_PRODUCT_ID,
    .version            = BLE_VERSION,
    .device_name        = DEVICE_NAME,
    .manufacturer_name  = DEVICE_MANUFACTURER,
    .serial_number      = DEVICE_SERIAL_NUMBER,
    .report_maps        = ble_report_maps,
    .report_maps_len    = 1,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGI(TAG, "Advertising started");
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        for (int i = 0; i < ESP_BD_ADDR_LEN; i++) {
            ESP_LOGD(TAG, "%x:", param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        s_sec_conn = true;
        memcpy(s_remote_bda, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",
                 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                 (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
        }
        break;

    /* 取消配对完成回调（由 bridge_request_unpair() 触发）
     * 配对信息清除成功后，主动断开连接，断开后会自动重新广播
     */
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
        if (param->remove_bond_dev_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "Bond removed, disconnecting device");
            esp_ble_gap_disconnect(param->remove_bond_dev_cmpl.bd_addr);
        } else {
            ESP_LOGE(TAG, "Failed to remove bond: 0x%x", param->remove_bond_dev_cmpl.status);
        }
        break;

    default:
        break;
    }
}

static void hidd_event_callback(void *handler_args, esp_event_base_t base,
                                int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    (void)handler_args;
    (void)base;

    switch (event) {

    case ESP_HIDD_START_EVENT:
        ESP_LOGI(TAG, "HIDD START");
        esp_ble_gap_set_device_name(s_device_name);
        esp_ble_gap_config_adv_data(&hidd_adv_data);
        break;

    case ESP_HIDD_CONNECT_EVENT:
        ESP_LOGI(TAG, "HIDD CONNECT");
        bridge_on_ble_connected();
        break;

    case ESP_HIDD_DISCONNECT_EVENT:
        s_sec_conn = false;
        memset(s_remote_bda, 0, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "HIDD DISCONNECT");
        bridge_on_ble_disconnected();
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;

    /* BLE 主机写入 LED Output Report（CapsLock/NumLock/ScrollLock 状态）
     * 转发到 USB 键盘，实现主机端 LED 状态同步
     */
    case ESP_HIDD_OUTPUT_EVENT:
        ESP_LOGD(TAG, "HIDD OUTPUT[%u] ID: %u, Len: %d",
                 param->output.map_index, param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        if (param->output.length >= 1) {
            bridge_handle_led_report(param->output.data[0]);
        }
        break;

    /* 主机切换协议模式（BOOT/REPORT），当前仅记录日志 */
    case ESP_HIDD_PROTOCOL_MODE_EVENT:
        ESP_LOGD(TAG, "HIDD PROTOCOL MODE[%u]: %s",
                 param->protocol_mode.map_index,
                 param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;

    default:
        break;
    }
}

esp_err_t ble_hid_manager_init(void)
{
    esp_err_t ret;

    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret) {
        ESP_LOGE(TAG, "release classic BT memory failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "initialize controller failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "enable controller failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "init bluedroid failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "enable bluedroid failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 注册 GATT 事件回调（esp_hidd 内部管理 HID GATT 服务创建与属性表） */
    esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);

    /* 将设备名同步到配置后，初始化 esp_hid BLE HID 设备 */
    ble_hid_config.device_name = s_device_name;
    ret = esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE,
                            hidd_event_callback, &s_hid_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init hidd device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    return ESP_OK;
}

esp_err_t ble_hid_manager_deinit(void)
{
    if (s_hid_dev) {
        esp_hidd_dev_deinit(s_hid_dev);
        s_hid_dev = NULL;
    }
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    return ESP_OK;
}

bool ble_hid_is_connected(void)
{
    if (s_hid_dev) {
        return esp_hidd_dev_connected(s_hid_dev);
    }
    return false;
}

const esp_bd_addr_t *ble_hid_get_remote_addr(void)
{
    return &s_remote_bda;
}

/* 返回 esp_hid 设备句柄，供 ble_hid_report 发送输入报告 */
esp_hidd_dev_t *ble_hid_get_hid_dev(void)
{
    return s_hid_dev;
}

void ble_hid_set_device_name(const char *name)
{
    snprintf(s_device_name_buf, sizeof(s_device_name_buf), "%s", name);
    s_device_name = s_device_name_buf;
    esp_ble_gap_set_device_name(s_device_name);
}
