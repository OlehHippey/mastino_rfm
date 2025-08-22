/*
 * mqttfunc.c
 *
 *  Created on: 10 трав. 2025 р.
 *      Author: voltekel
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "global.h"
#include "function.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mainfunc.h"
#include "config.h"
#include "cJSON.h"

static const char *TAG = "smart_home_mqtt";

//================= Reports and Publics =====================================================
static void publish_sensor_data(const char *sensor, bool value) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/sensors/%s", DEVICE_ID, sensor);

    char payload[256];
    snprintf(payload, sizeof(payload), "{\"uptime\":%lld,\"sensor\":\"%s\",\"value\":%s}",
             time(NULL), sensor, value ? "true" : "false");
    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
}
//--------------------------------------------------------------------------------------------------
static void report_actuator_state(const char *target, const char *state) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/report/%s", DEVICE_ID, target);

    char payload[128];
    snprintf(payload, sizeof(payload), "{\"value\":\"%s\"}", state);
    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
}

//========================= Main MQTT Functions =========================================
static void handle_command(const char *cmd, const char *target, const char *value) {
	ESP_LOGW(TAG, "Start handle_command: cmd=%s, target=%s, value=%s", cmd, target, value);
    if (strcmp(cmd, "set") == 0 && strcmp(target, "water_valve") == 0) {

        if (strcmp(value, "open") == 0) {
            reset_alarm();
            report_actuator_state("water_valve", "open");
        } else if (strcmp(value, "close") == 0) {
            cfg->close_valve |= VST_CLOSE_VALVE_CMD;
            report_actuator_state("water_valve", "close");
        }
    } else if (strcmp(cmd, "get") == 0) {
        if (strcmp(target, "status") == 0) {
			update_power=true;
			update_sensors=true;
			update_inputs=true;
			update_actuators=true;
            update_state=true;
        } else if (strcmp(target, "config") == 0) {
            update_config=true;
        }
    } else if (strcmp(cmd, "upload") == 0 && strcmp(target, "firmware") == 0) {
		ESP_LOGW(TAG, "Firmware command: cmd=%s, target=%s, value=%s", cmd, target, value);
        // start_firmware_update();
    } else {
        ESP_LOGW(TAG, "Unknown command: cmd=%s, target=%s, value=%s", cmd, target, value);
    }
}
//--------------------------------------------------------------------------------------------------
static void handle_config_update(const char *json_str, bool isWIFI) {
    cJSON *root = cJSON_Parse(json_str);
    if (root) {
        apply_device_config_from_json(root, isWIFI);
        cJSON_Delete(root);
        // ACK
        char topic[128];
        snprintf(topic, sizeof(topic), "home/%s/config/set/ack", DEVICE_ID);
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"result\":\"ok\",\"uptime\":%lld}", time(NULL));
        esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
    } else {
        ESP_LOGE(TAG, "Invalid config JSON");
    }
}

//--------------------------------------------------------------------------------------------------
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "Connected to broker");
            char topic_ctrl[128], topic_cfg[64];
            snprintf(topic_ctrl, sizeof(topic_ctrl), "home/%s/control/+", DEVICE_ID);
            esp_mqtt_client_subscribe(clientMQTT, topic_ctrl, 1);
            snprintf(topic_cfg, sizeof(topic_cfg), "home/%s/config/set", DEVICE_ID);
            esp_mqtt_client_subscribe(clientMQTT, topic_cfg, 1);
            snprintf(topic_cfg, sizeof(topic_cfg), "home/%s/config/wifi", DEVICE_ID);
            esp_mqtt_client_subscribe(clientMQTT, topic_cfg, 1);
            publish_sensor_data("water_leak", false);
            break;
        }
        case MQTT_EVENT_DATA: {
            char *topic = strndup(event->topic, event->topic_len);
            char *data = strndup(event->data, event->data_len);
            ESP_LOGW(TAG, "Incoming message on topic: %s\nPayload: %s", topic, data);

            if (strstr(topic, "/control/")) {
                cJSON *root = cJSON_Parse(data);
                if (root) {
                    const cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                    const cJSON *target = cJSON_GetObjectItem(root, "target");
                    const cJSON *value = cJSON_GetObjectItem(root, "value");
                    if (cmd && target && value && cJSON_IsString(cmd) && cJSON_IsString(target) && cJSON_IsString(value)) {
                        handle_command(cmd->valuestring, target->valuestring, value->valuestring);
                    }
                    cJSON_Delete(root);
                }
            } else if (strstr(topic, "/config/wifi")) {
				ESP_LOGW(TAG, "Incoming config WIFI topic: %s ", topic);
                handle_config_update(data, true);
            }  else if (strstr(topic, "/config/set")) {
				ESP_LOGW(TAG, "Incoming config on topic: %s ", topic);
                handle_config_update(data, false);
            } else {
				ESP_LOGW(TAG, "Incoming Unknown topic: %s ", topic);
			}
            free(topic);
            free(data);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from broker");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed successfully, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Published message, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR: %d", event->error_handle->error_type);
            break;
        default:
            ESP_LOGI(TAG, "Other MQTT event id: %d", event->event_id);
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
        .broker.address.uri = URL_MASTINO_MQTT, 
        // "mqtt://s2.bsd.com.ua:7367", //178.94.173.180 //178.94.173.181  
        .credentials.username = mqtt_username,
        .credentials.authentication.password = mac_str, 
       // .credentials.client_id=DEVICE_ID,
      //  .network.buffer_size = 4096,
      //  .network.out_buffer_size = 4096,
      //  .network.in_buffer_size = 4096,
        .network.disable_auto_reconnect = false,
        .task.stack_size = 8192,
    };
    clientMQTT = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(clientMQTT, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(clientMQTT);
}

void mqttcon(void) {
    ESP_LOGI(TAG, "[APP] MQTT Init...");
    mqtt_app_start();
}
