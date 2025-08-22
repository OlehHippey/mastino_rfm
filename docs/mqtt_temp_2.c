// mqttfunc.c
#include "mqttfunc.h"
#include "esp_log.h"
#include "cJSON.h"
#include "global.h"

static const char *TAG = "MQTT_FUNC";
static esp_mqtt_client_handle_t client = NULL;

void mqtt_publish_sensor_state(uint8_t num) {
    if (!sensors[num].pres) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/%s/%s/sensors/%u", HOME_ID, ROOM, DEVICE_ID, num + 1);

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"rssi\":%d,\"voltage\":%.1f,\"status\":%u,\"last_time\":%llu}",
             sensors[num].name,
             sensors[num].rssi,
             (float)sensors[num].voltage / 10,
             sensors[num].status,
             sensors[num].last_time);

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
}

void mqtt_publish_all_sensors(void) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i].pres) {
            mqtt_publish_sensor_state(i);
        }
    }
}

void mqtt_publish_valve_state(bool is_closed) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/%s/%s/valve/status", HOME_ID, ROOM, DEVICE_ID);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"closed\":%s}", is_closed ? "true" : "false");

    esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
}

static void handle_control_message(const char *data) {
    cJSON *root = cJSON_Parse(data);
    if (!root) return;

    const cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    const cJSON *target = cJSON_GetObjectItem(root, "target");
    const cJSON *value = cJSON_GetObjectItem(root, "value");

    if (cmd && target && value && strcmp(cmd->valuestring, "set") == 0) {
        if (strcmp(target->valuestring, "water_valve") == 0) {
            if (strcmp(value->valuestring, "open") == 0) {
                reset_alarm();
                mqtt_publish_valve_state(false);
                ESP_LOGI(TAG, "Valve opened via MQTT");
            } else if (strcmp(value->valuestring, "close") == 0) {
                cfg->close_valve |= VST_CLOSE_VALVE_CMD;
                mqtt_publish_valve_state(true);
                ESP_LOGI(TAG, "Valve closed via MQTT");
            }
        }
    }

    cJSON_Delete(root);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to broker");
            esp_mqtt_client_subscribe(client, "home/+/+/+/control/+", 1);
            mqtt_publish_valve_state(cfg->close_valve != 0);
            mqtt_publish_all_sensors();
            break;
        case MQTT_EVENT_DATA: {
            char *data = strndup(event->data, event->data_len);
            handle_control_message(data);
            free(data);
            break;
        }
        default:
            break;
    }
}

void mqtt_app_start(void) {
    char mqtt_username[64];
    snprintf(mqtt_username, sizeof(mqtt_username),
             "Mastino_%02X%02X%02X%02X%02X%02X",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://s2.bsd.com.ua:7367",
        .credentials.username = mqtt_username,
        .credentials.authentication.password = mac_str,
        .task.stack_size = 8192,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}