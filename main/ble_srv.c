#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_log.h"
#include "cJSON.h"

#include "global.h"
#include "function.h"
#include "mainfunc.h"
#include "local_handle.h"

#define BLE_STACK_SIZE (8192)

//static TaskHandle_t ble_task_handle;

static const char *TAG = "BLE";

// UUIDs
static ble_uuid16_t uuid_service  = BLE_UUID16_INIT(0xFF00);
static ble_uuid16_t uuid_config   = BLE_UUID16_INIT(0xFF01);
static ble_uuid16_t uuid_sensor   = BLE_UUID16_INIT(0xFF02);
static ble_uuid16_t uuid_command  = BLE_UUID16_INIT(0xFF03);

// Handles
static uint16_t config_handle = 0;
static uint16_t sensor_handle = 0;
static uint16_t command_handle = 0;

static uint16_t current_conn_handle = BLE_HS_CONN_HANDLE_NONE;


// Prototypes
static void start_advertising(void);

// Advertising parameters
static const struct ble_gap_adv_params adv_params = {
    .conn_mode = BLE_GAP_CONN_MODE_UND,
    .disc_mode = BLE_GAP_DISC_MODE_GEN,
};


//------------------- Send notify with JSON response -----------------------
static int gatt_command_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;

    char buf[256];
    int len = OS_MBUF_PKTLEN(ctxt->om);
    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, NULL);
    buf[len] = '\0';
    ESP_LOGI(TAG, "Received: %s", buf);

    // Обробка JSON через загальну функцію
    char reply_buf[512] = {0};
    handle_command_json(buf, reply_buf, sizeof(reply_buf));

    if (current_conn_handle != BLE_HS_CONN_HANDLE_NONE && sensor_handle != 0) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(reply_buf, strlen(reply_buf));
        ble_gattc_notify_custom(current_conn_handle, sensor_handle, om);
        ESP_LOGI(TAG, "Notify sent: %s", reply_buf);
    }

    return 0;
}


static int gatt_dummy_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "GATT access on handle %d, op=%d", attr_handle, ctxt->op);
    return 0;
}

// GAP callback
static int gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                current_conn_handle = event->connect.conn_handle;
                struct ble_gap_conn_desc desc;
                if (ble_gap_conn_find(event->connect.conn_handle, &desc) == 0) {
                    const uint8_t *addr = desc.peer_id_addr.val;
                    ESP_LOGI(TAG, "Connected to: %02X:%02X:%02X:%02X:%02X:%02X",
                             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                }
            } else {
                ESP_LOGW(TAG, "Connect failed, restarting advertising");
                start_advertising();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected, restarting advertising");
            current_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            start_advertising();
            break;
        default:
            break;
    }
    return 0;
}

static void start_advertising(void) {
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ble_uuid16_t svc_uuid = BLE_UUID16_INIT(0xFF00);
    fields.uuids16 = &svc_uuid;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    uint8_t own_addr_type;
    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, gap_event_cb, NULL);
    ESP_LOGI(TAG, "Advertising started");
}

static void gatt_event(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_REGISTER_OP_CHR) {
        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(ctxt->chr.chr_def->uuid, uuid_str);
        ESP_LOGI(TAG, "Registered char UUID: %s, handle: %d", uuid_str, ctxt->chr.val_handle);

        if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &uuid_config.u) == 0) {
            config_handle = ctxt->chr.val_handle;
        } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &uuid_sensor.u) == 0) {
            sensor_handle = ctxt->chr.val_handle;
        } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &uuid_command.u) == 0) {
            command_handle = ctxt->chr.val_handle;
        }
    }
}

static void gatt_init(void) {
    ble_svc_gap_init();
    ble_svc_gatt_init();

    static struct ble_gatt_chr_def chr_defs[] = {
        {
            .uuid = &uuid_config.u,
            .access_cb = gatt_dummy_access_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
        },
        {
            .uuid = &uuid_sensor.u,
            .access_cb = gatt_dummy_access_cb,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        },
        {
            .uuid = &uuid_command.u,
            .access_cb = gatt_command_access_cb,
            .flags = BLE_GATT_CHR_F_WRITE,
        },
        {0},
    };

    static struct ble_gatt_svc_def gatt_svcs[] = {
        {
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = &uuid_service.u,
            .characteristics = chr_defs,
        },
        {0},
    };

    ble_hs_cfg.gatts_register_cb = gatt_event;
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
}

static void on_sync(void) {
    ESP_LOGI(TAG, "BLE sync");
    ble_svc_gap_device_name_set("MastinoBLE");
    gatt_init();
    ble_gatts_start();
    start_advertising();
}

void ble_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
    vTaskDelete(NULL);
}

void ble_start(void) {
    ESP_LOGI(TAG, "Starting BLE");
    nimble_port_init();
    ble_hs_cfg.sync_cb = on_sync;
    nimble_port_freertos_init(ble_task);
  //  nimble_port_freertos_init_with_task(ble_task, NULL, BLE_STACK_SIZE, &ble_task_handle);
}