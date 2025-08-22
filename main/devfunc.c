/*
 * devfunc.c
 *
 *  Created on: 15 мая 2025 г.
 *      Author: ProgMen
 */

#include "cJSON.h"
#include "esp_log.h"
#include "function.h"
#include "global.h"
#include "devfunc.h"
#include "lwip/sockets.h"
#include "mqtt_client.h"
#include <stdbool.h>
#include <math.h>  // для isfinite()

#include "dnsmulti.h"
#include "dnsudp.h"

#define TAG "DEVICE_PROC"
 

int send_config=0;

//============================ Basic Sensors Functions ==============================================
void mqtt_publish_heartbeat(void) {
    if (!clientMQTT) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/heartbeat", DEVICE_ID);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"uptime\":%llu}", gettimesec());

    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 0, 0);
}

//---------------------------------------------------------------------------------------------------
void mqtt_publish_sensor_state(uint8_t num) {
	a_log(TAG, "mqtt_publish_sensor_state (clientMQTT=%d): %d", clientMQTT, sensors[num].pres);
	if (!clientMQTT) return;
    if (!sensors[num].pres) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/sensors/%u", DEVICE_ID, num + 1);

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"rssi\":%d,\"voltage\":%.1f,\"status\":%u,\"last_time\":%llu}",
             sensors[num].name,
             sensors[num].rssi,
             (float)sensors[num].voltage / 10,
             sensors[num].status,
             sensors[num].last_time);

    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
}

//--------------------------------------------------------------------------------------------------
void mqtt_publish_valve_state(bool is_closed) {
	if (!clientMQTT) return;
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/actuators/state", DEVICE_ID);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"closed\":%s}", is_closed ? "true" : "false");

    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
}
//--------------------------------------------------------------------------------------------------
void mqtt_publish_zones_state() {
    if (!clientMQTT) return;
    bool zone1_trouble=(inputs_state >> SEND_ALERT_ZONE1) & 1; 
    bool zone2_trouble=(inputs_state >> SEND_ALERT_ZONE2) & 1; 
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/zones/state", DEVICE_ID);

    char payload[256];
    snprintf(payload, sizeof(payload),
        "{\"zones\":["
            "{\"id\":1,\"trouble\":%s},"
            "{\"id\":2,\"trouble\":%s}"
        "]}",
        zone1_trouble ? "true" : "false",
        zone2_trouble ? "true" : "false"
    );

    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
}
//--------------------------------------------------------------------------------------------------
void mqtt_publish_perif_state(void) {
	if (!clientMQTT) return;
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/report", DEVICE_ID);

    char payload[1024] = {0};
    snprintf(payload, sizeof(payload), "{\"valve\":%u,\"up\":%llu,\"sensors\":[",
             cfg->close_valve != 0,
             gettimesec());

    int first = 1;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].pres) continue;

        char entry[256];
        snprintf(entry, sizeof(entry),
                 "%s{\"nm\":%u,\"na\":\"%s\",\"rs\":%d,\"v\":%u,\"lt\":%llu,\"s\":%u}",
                 first ? "" : ",",
                 i + 1, sensors[i].name,
                 sensors[i].rssi, sensors[i].voltage,
                 sensors[i].last_time, sensors[i].status);

        strncat(payload, entry, sizeof(payload) - strlen(payload) - 1);
        first = 0;
    }
    strncat(payload, "]}", sizeof(payload) - strlen(payload) - 1);

    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
    a_log(TAG, "Published full device state");
}
//-------------------------------------------------------------------------------------------------
void mqtt_publish_all_sensors(void) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i].pres) {
            mqtt_publish_sensor_state(i);
        }
    }
}
//--------------------------------------------------------------------------------------------------
void mqtt_publish_device_config(void) {
    if (!clientMQTT) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/config", DEVICE_ID);

    char payload[1024];
    snprintf(payload, sizeof(payload),
        "{"
        "\"wifi_mode\":%u,"
        "\"dev_name\":\"%s\","
        "\"mac_addr\":\"%s\","
        "\"version\":\"%u.%u.%u\","
        "\"cl_ssid\":\"%s\","
        "\"cl_key\":\"%s\","
        "\"ntp_server\":\"%s\","
        "\"voff\":%.3f,"
        "\"perif_state\":%u,"
        "\"wifi_state\":%u"
        "}",
        cfg->wifi_mode,
        cfg->dev_name,
        mac_str,
        cfg->vmj, cfg->vmn, cfg->vadd,
        cfg->cl_ssid,
        cfg->cl_key,
        cfg->ntp_server,
        cfg->voff,
        cfg->perifstate,
        wifi_state
    );

    int msg_id = esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
    a_log(TAG, "Published device config (msg_id=%d): %s", msg_id, payload);
}

//--------------------------------------------------------------------------------------------------


void mqtt_publish_device_status(void) {
    if (!clientMQTT) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/status", DEVICE_ID);

    if (!isfinite(vbat)) vbat = 0;
    if (!isfinite(vin)) vin = 0;

    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddNumberToObject(root, "uptime", gettimesec());
    cJSON_AddNumberToObject(root, "vbat", vbat);
    cJSON_AddNumberToObject(root, "vin", vin);

    cJSON_AddBoolToObject(root, "power_en_vin", (powerstate & ST_POWER_EN_VIN) != 0);
    cJSON_AddBoolToObject(root, "power_en_bat", (powerstate & ST_POWER_EN_BAT) != 0);
    cJSON_AddBoolToObject(root, "power_charge", (powerstate & ST_POWER_CHARGE) != 0);
    cJSON_AddBoolToObject(root, "power_bat_present", (powerstate & ST_POWER_BAT_PRESENT) != 0);
    cJSON_AddBoolToObject(root, "power_bat_error", (powerstate & ST_POWER_BAT_ERROR) != 0);

    char *payload = cJSON_PrintUnformatted(root);
    if (payload) {
        int msg_id = esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
        a_log(TAG, "Published device status (msg_id=%d): %s", msg_id, payload);
        free(payload);
    } else {
        a_log(TAG, "Failed to generate device status JSON");
    }

    cJSON_Delete(root);
}
//--------------------------------------------------------------------------------------------------
void mqtt_send_alert(uint16_t alert) {
	if (!clientMQTT) return;
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/alert", DEVICE_ID);

    char payload[64];

    snprintf(payload, sizeof(payload), "{\"uptime\":%llu,\"alert\":%u}", gettimesec(), alert);


    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
    a_log(TAG, "Alert published: %u", alert);
}
//============================= Sensor Analizate Functions =========================================

void chk_mqtt_onesec(void) {
    static uint8_t counter = 0;
  
    if (!clientMQTT){ 
        a_log(TAG, "----------------------- Start chk_mqtt_onesec clientMQTT ----------------------------");
        return;
    } 
    
    if (update_power){
		counter = 0;
		update_power=false;
		ESP_LOGE(TAG, "Published Power status changed: %d", lpower_state);
		mqtt_publish_device_status();
	} else if (update_state){
		counter = 0;
		update_state=false;
		mqtt_publish_perif_state();
	} else 
	if (update_sensors){
	//	a_log(TAG, "Zones mqtt_publish_all_sensors (update_sensors=%d): %d", lzones_state, cfg->zone_state);
		counter = 0;
		update_sensors=false;
		 mqtt_publish_all_sensors();
	} else 
	if (update_inputs){
		counter = 0;
		update_inputs=false;
		mqtt_publish_zones_state();
	} else 
	if (update_actuators){
		counter = 0;
		update_actuators=false;
		mqtt_publish_valve_state(cfg->close_valve != 0);
	} else
	if (update_config){
		counter = 0;
		update_config=false;
		 mqtt_publish_device_config();
	}
	
	for (int i=0; i<NUM_ALERTS; i++){
		if (alerts_list[i]){
			mqtt_send_alert(alerts_list[i]);
			alerts_list[i]=0;
		}
	}
	
	if (mdns_srv_started==1) start_mdns_service("mastino", "Mastino Device"); 
  	if (mdns_udp_started==1) start_udp_server();

    if (++counter >= 60) {
		a_log(TAG, "Start heatbeat full device status");
        mqtt_publish_heartbeat();
        counter = 0;
      //  start_mdns_service("mastino", "Mastino Device");
      //  start_udp_server();
    }
}
//--------------------------------------------------------------------------------------------------
#ifdef USE_SENSORS
void mqtt_publish_sensor_event(uint8_t num) {
    if (num >= SENSOR_COUNT || !sensors[num].pres) return;
    mqtt_publish_sensor_state(num);
}
#endif
//--------------------------------------------------------------------------------------------------
void mqtt_send_valve_update(bool closed) {
    mqtt_publish_valve_state(closed);
}

//--------------------------------------------------------------------------------------------------
void mqtt_send_log(const char *message) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/log", DEVICE_ID);

    esp_mqtt_client_publish(clientMQTT, topic, message, 0, 0, 0);
    a_log(TAG, "Log: %s", message);
}
//--------------------------------------------------------------------------------------------------
void mqtt_send_devconf(void) {
    char topic[128];
    snprintf(topic, sizeof(topic), "home/%s/config", DEVICE_ID);

    char payload[512];
snprintf(payload, sizeof(payload),
    "{"
    "\"wifi_mode\":%u,"
    "\"cl_ssid\":\"%s\","
    "\"cl_key\":\"%s\","
    "\"dev_name\":\"%s\","
    "\"ntp_server\":\"%s\","
    "\"version\":\"%u.%u.%u\""
    "}",
    cfg->wifi_mode,
    cfg->cl_ssid,
    cfg->cl_key,
    cfg->dev_name,
    cfg->ntp_server,
    cfg->vmj, cfg->vmn, cfg->vadd);


    esp_mqtt_client_publish(clientMQTT, topic, payload, 0, 1, 0);
    a_log(TAG, "Device config published");
    send_config = 1;
}

//--------------------------------------------------------------------------------------------------
void mqtt_client_task(void *pvParameters) {
    a_log(TAG, "MQTT client task started (simulated UDP client logic)");

    while (1) {
        chk_mqtt_onesec();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} 


