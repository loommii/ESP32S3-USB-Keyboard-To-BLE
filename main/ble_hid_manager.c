#include <string.h>
#include "esp_log.h"
#include "esp_hidd.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_store.h"
#include "host/ble_sm.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/bas/ble_svc_bas.h"
#include "ble_hid_manager.h"
#include "bridge.h"
#include "led_status.h"
#include "app_main.h"
#include "config.h"

static const char *TAG = "BLE_HID";

/*--------------------------------------------------------------------*/
/*  STATIC VARIABLES                                                  */
/*--------------------------------------------------------------------*/

/* BLE 连接状态 */
static bool s_sec_conn = false;             /* 是否已建立 BLE 连接 */
static uint8_t s_remote_bda[6] = {0};       /* 对端设备蓝牙地址 */
static uint8_t s_peer_addr_type = 0;        /* 对端地址类型（公共/随机） */
static uint16_t s_conn_handle = 0;          /* 当前连接句柄 */
static esp_hidd_dev_t *s_hid_dev = NULL;    /* esp_hid 组件设备句柄 */
static char s_device_name_buf[32] = DEVICE_NAME_1;  /* 当前广播设备名 */
static uint8_t s_own_addr_type = 0;         /* 本地地址类型 */
/* 配对状态：true = 正在等待用户通过键盘输入配对码 */
static bool s_pairing_active = false;

/*--------------------------------------------------------------------*/
/*  REPORT MAP                                                        */
/*--------------------------------------------------------------------*/

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

/* BLE HID 设备配置：VID / PID / 版本 / report map */
static esp_hid_device_config_t ble_hid_config = {
    .vendor_id         = BLE_VENDOR_ID,
    .product_id        = BLE_PRODUCT_ID,
    .version           = BLE_VERSION,
    .device_name       = DEVICE_NAME_1,
    .manufacturer_name = DEVICE_MANUFACTURER,
    .serial_number     = DEVICE_SERIAL_NUMBER,
    .report_maps       = ble_report_maps,
    .report_maps_len   = 1,
};

/*--------------------------------------------------------------------*/
/*  前向声明                                                            */
/*--------------------------------------------------------------------*/

static int nimble_gap_event(struct ble_gap_event *event, void *arg);
static void start_advertising(void);

/*--------------------------------------------------------------------*/
/*  广播                                                                  */
/*--------------------------------------------------------------------*/

/* 启动 BLE 广播，连接回调注册在 ble_gap_adv_start 中 */
static void start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.name = (uint8_t *)s_device_name_buf;
    fields.name_len = strlen(s_device_name_buf);
    fields.name_is_complete = 1;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.appearance = BLE_APPEARANCE_HID_KEYBOARD;
    fields.appearance_is_present = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    ble_uuid16_t uuid16 = BLE_UUID16_INIT(0x1812);
    fields.uuids16 = &uuid16;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv set fields failed: %d", rc);
        return;
    }

    /* Scan Response */
    struct ble_hs_adv_fields scan_rsp;
    memset(&scan_rsp, 0, sizeof(scan_rsp));
    scan_rsp.name = (uint8_t *)s_device_name_buf;
    scan_rsp.name_len = strlen(s_device_name_buf);
    scan_rsp.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&scan_rsp);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv rsp set fields failed: %d", rc);
    }

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, nimble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv start failed: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising started");
}

/*--------------------------------------------------------------------*/
/*  GAP 事件回调（NimBLE 协议栈）——连接、配对、加密变更等核心事件              */
/*  注意：PASSKEY_ACTION / REPEAT_PAIRING 只走此回调，不走 event_listener  */
/*--------------------------------------------------------------------*/

static int nimble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    /* 连接建立：保存对端地址和句柄，通知桥接层 */
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            struct ble_gap_conn_desc desc;
            int rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (rc == 0) {
                s_peer_addr_type = desc.peer_id_addr.type;
                memcpy(s_remote_bda, desc.peer_id_addr.val, 6);
            }
            s_sec_conn = true;
            s_conn_handle = event->connect.conn_handle;
            update_led_status();
            bridge_on_ble_connected();
            ESP_LOGI(TAG, "BLE connected, conn_handle=%d", s_conn_handle);
        } else {
            start_advertising();
        }
        return 0;

    /* 连接断开：清除配对状态，恢复广播 */
    case BLE_GAP_EVENT_DISCONNECT:
        if (s_pairing_active) {
            s_pairing_active = false;
            bridge_on_pairing_done(false);
        }
        s_sec_conn = false;
        s_conn_handle = 0;
        memset(s_remote_bda, 0, 6);
        update_led_status();
        bridge_on_ble_disconnected();
        ESP_LOGI(TAG, "BLE disconnected, reason=%d", event->disconnect.reason);
        start_advertising();
        return 0;

    /* 连接参数更新（忽略） */
    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGD(TAG, "Connection parameters updated");
        return 0;

    /* 配对密令（Passkey）动作请求：
     *   BLE_SM_IOACT_INPUT  -> 主机显示配对码，等待键盘输入
     *   BLE_SM_IOACT_NUMCMP -> 数字比对（自动接受）
     *   BLE_SM_IOACT_DISP   -> 本应本地显示（Keyboard-Only 不适用） */
    case BLE_GAP_EVENT_PASSKEY_ACTION: {
        uint8_t action = event->passkey.params.action;
        ESP_LOGI(TAG, "Passkey action: %d", action);

        if (action == BLE_SM_IOACT_INPUT) {
            s_pairing_active = true;
            bridge_on_pairing_start();
        } else if (action == BLE_SM_IOACT_NUMCMP) {
            struct ble_sm_io io = {0};
            io.action = BLE_SM_IOACT_NUMCMP;
            io.numcmp_accept = 1;
            ble_sm_inject_io(event->passkey.conn_handle, &io);
        } else if (action == BLE_SM_IOACT_DISP) {
            ESP_LOGI(TAG, "Display passkey (not applicable for keyboard-only)");
        }
        return 0;
    }

    /* 加密状态变更：status==0 表示配对成功 */
    case BLE_GAP_EVENT_ENC_CHANGE: {
        int enc_status = event->enc_change.status;
        ESP_LOGI(TAG, "Encryption changed, status=%d", enc_status);
        if (s_pairing_active) {
            s_pairing_active = false;
            bridge_on_pairing_done(enc_status == 0);
        }
        return 0;
    }

    /* 重复配对：对端已有旧绑定，删除后重新配对 */
    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        if (rc == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        ESP_LOGI(TAG, "Repeat pairing - deleting old bond, retrying");
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    default:
        return 0;
    }
}

/*--------------------------------------------------------------------*/
/*  HID 事件回调（收发报告、连接/断开通知）                                    */
/*--------------------------------------------------------------------*/

static void hidd_event_callback(void *handler_args, esp_event_base_t base,
                                int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    (void)handler_args;
    (void)base;

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(TAG, "HIDD START");

        ble_hid_config.device_name = s_device_name_buf;
        ble_svc_gap_device_name_set(s_device_name_buf);

        int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
        if (rc != 0) {
            ESP_LOGE(TAG, "infer addr type failed: %d", rc);
        }

        start_advertising();
        break;
    }

    case ESP_HIDD_CONNECT_EVENT:
        ESP_LOGI(TAG, "HIDD CONNECT");
        break;

    case ESP_HIDD_DISCONNECT_EVENT:
        ESP_LOGI(TAG, "HIDD DISCONNECT (reason=%d)", param->disconnect.reason);
        break;

    case ESP_HIDD_OUTPUT_EVENT:
        ESP_LOGD(TAG, "HIDD OUTPUT[%u] ID: %u, Len: %d",
                 param->output.map_index, param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        if (param->output.length >= 1) {
            bridge_handle_led_report(param->output.data[0]);
        }
        break;

    case ESP_HIDD_PROTOCOL_MODE_EVENT:
        ESP_LOGD(TAG, "HIDD PROTOCOL MODE[%u]: %s",
                 param->protocol_mode.map_index,
                 param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;

    default:
        break;
    }
}

/*--------------------------------------------------------------------*/
/*  HOST TASK                                                         */
/*--------------------------------------------------------------------*/

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/*--------------------------------------------------------------------*/
/*  PUBLIC API                                                        */
/*--------------------------------------------------------------------*/

/* 初始化 BLE HID + 安全配置（Keyboard-Only + MITM + 安全连接） */
esp_err_t ble_hid_manager_init(void)
{
    ESP_LOGI(TAG, "BLE HID Manager init (NimBLE)");

    nimble_port_init();

    /* 安全配置：Keyboard-Only I/O 能力，MITM 保护，SC 安全连接，支持绑定 */
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_KEYBOARD_ONLY;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    ESP_LOGI(TAG, "Initializing HID Device (esp_hidd)");
    esp_err_t ret = esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE,
                                      hidd_event_callback, &s_hid_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init hidd device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 电池服务由 nimble_hidd.c 内部初始化，此处仅设值为 100%（外部供电） */
    ble_svc_bas_battery_level_set(BATTERY_LEVEL);

    nimble_port_freertos_init(ble_host_task);

    return ESP_OK;
}

esp_err_t ble_hid_manager_deinit(void)
{
    if (s_hid_dev) {
        esp_hidd_dev_deinit(s_hid_dev);
        s_hid_dev = NULL;
    }
    s_sec_conn = false;
    memset(s_remote_bda, 0, 6);
    return ESP_OK;
}

bool ble_hid_is_connected(void)
{
    if (s_hid_dev) {
        return esp_hidd_dev_connected(s_hid_dev);
    }
    return false;
}

void ble_hid_get_remote_addr(uint8_t addr[6])
{
    memcpy(addr, s_remote_bda, 6);
}

esp_hidd_dev_t *ble_hid_get_hid_dev(void)
{
    return s_hid_dev;
}

void ble_hid_set_device_name(const char *name)
{
    snprintf(s_device_name_buf, sizeof(s_device_name_buf), "%s", name);
}

/* 主动断开当前 BLE 连接 */
esp_err_t ble_hid_disconnect(void)
{
    if (!s_sec_conn || s_conn_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    int rc = ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (rc != 0) {
        ESP_LOGE(TAG, "disconnect failed: %d", rc);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* 删除对端绑定信息（断开连接 + 清除存储的密钥） */
esp_err_t ble_hid_remove_bond(void)
{
    if (s_sec_conn && s_conn_handle != 0) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    ble_addr_t peer;
    memcpy(peer.val, s_remote_bda, 6);
    peer.type = s_peer_addr_type;

    if (memcmp(s_remote_bda, "\x00\x00\x00\x00\x00\x00", 6) == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    int rc = ble_store_util_delete_peer(&peer);
    if (rc != 0 && rc != BLE_HS_ENOTCONN) {
        ESP_LOGD(TAG, "remove bond returned: %d", rc);
    }

    memset(s_remote_bda, 0, 6);
    s_peer_addr_type = 0;
    return ESP_OK;
}

/* 查询是否正在等待用户输入配对码 */
bool ble_hid_is_pairing(void)
{
    return s_pairing_active;
}

/* 向 NimBLE 协议栈注入用户通过键盘输入的 6 位配对码 */
void ble_hid_inject_passkey(uint32_t passkey)
{
    if (!s_pairing_active) {
        return;
    }

    if (s_conn_handle == 0) {
        s_pairing_active = false;
        bridge_on_pairing_done(false);
        return;
    }

    struct ble_sm_io io = {0};
    io.action = BLE_SM_IOACT_INPUT;
    io.passkey = passkey;

    int rc = ble_sm_inject_io(s_conn_handle, &io);
    if (rc == 0) {
        ESP_LOGI(TAG, "Passkey %06lu injected", (unsigned long)passkey);
    } else {
        ESP_LOGE(TAG, "Inject passkey failed: %d", rc);
        s_pairing_active = false;
        bridge_on_pairing_done(false);
    }
}

/* 取消配对：注入无效密令 0xFFFFFF 以终止配对流程 */
void ble_hid_cancel_pairing(void)
{
    if (!s_pairing_active) {
        return;
    }

    if (s_conn_handle != 0) {
        struct ble_sm_io io = {0};
        io.action = BLE_SM_IOACT_INPUT;
        io.passkey = 0xFFFFFF;
        ble_sm_inject_io(s_conn_handle, &io);
    }

    s_pairing_active = false;
    bridge_on_pairing_done(false);
}
