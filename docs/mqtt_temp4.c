// mqtt_sensors.c (колишній udp.c — адаптовано для MQTT)

#include "global.h"
#include "mqttfunc.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "MQTT_SENSORS";

void chk_mqtt_onesec(void) {
    static uint8_t counter = 0;

    if (!cfg->servmail[0] || !strchr(cfg->servmail, '@')) return;

    if (++counter >= 30) {
        mqtt_publish_all_sensors();
        mqtt_publish_valve_state(cfg->close_valve != 0);
        counter = 0;
    }
}

#ifdef USE_SENSORS
void mqtt_publish_sensor_event(uint8_t num) {
    if (num >= SENSOR_COUNT || !sensors[num].pres) return;
    mqtt_publish_sensor_state(num);
}
#endif

void mqtt_send_valve_update(bool closed) {
    mqtt_publish_valve_state(closed);
}

void mqtt_send_alert(uint16_t alert) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/%s/%s/alert", HOME_ID, ROOM, DEVICE_ID);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"alert\":%u}", alert);

    esp_mqtt_client_publish(NULL, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Alert published: %u", alert);
}

void mqtt_send_log(const char *message) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/%s/%s/log", HOME_ID, ROOM, DEVICE_ID);

    esp_mqtt_client_publish(NULL, topic, message, 0, 0, 0);
    ESP_LOGI(TAG, "Log: %s", message);
}

void mqtt_send_devconf(void) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/%s/%s/config", HOME_ID, ROOM, DEVICE_ID);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"wifi_mode\":%u,\"ap_channel\":%u,\"ap_ssid\":\"%s\",\"ap_key\":\"%s\","
             "\"cl_ssid\":\"%s\",\"cl_key\":\"%s\",\"dev_name\":\"%s\",\"ntp_server\":\"%s\",\"version\":\"%u.%u.%u\"}",
             cfg->wifi_mode,
             cfg->ap_channel,
             cfg->ap_ssid,
             cfg->ap_key,
             cfg->cl_ssid,
             cfg->cl_key,
             cfg->dev_name,
             cfg->ntp_server,
             cfg->vmj, cfg->vmn, cfg->vadd);

    esp_mqtt_client_publish(NULL, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Device config published");
    send_config = 1;
}

void mqtt_client_task(void *pvParameters) {
    char rx_buffer[1024];
    ESP_LOGI(TAG, "MQTT client task started (simulated UDP client logic)");

    while (1) {
        chk_mqtt_onesec();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} 
