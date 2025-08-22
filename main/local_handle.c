/*
 * local_handle.c
 *
 *  Created on: 3 июл. 2025 г.
 *      Author: ProgMen
 */

 
#include "cJSON.h"
#include "esp_log.h"
#include "global.h"
#include "function.h"
#include "mainfunc.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>


//===================== BASIC JSON FUNCTIONS ================================

void generate_status_json(char *buf, size_t buf_size) {
    snprintf(buf, buf_size,
        "{"
        "\"valve\":%u,\"up\":%llu,\"sensors\":["
        "{\"nm\":1,\"na\":\"Sensor1\",\"rs\":-70,\"v\":3300,\"lt\":123456,\"s\":1}"
        "]"
        "}",
        cfg->close_valve != 0,
        gettimesec()
    );
}


void generate_power_json(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        snprintf(buf, buf_size, "{\"error\":\"cjson_alloc\"}");
        return;
    }

    cJSON_AddNumberToObject(root, "uptime", gettimesec());
    cJSON_AddNumberToObject(root, "vbat", vbat);
    cJSON_AddNumberToObject(root, "vin", vin);

    cJSON_AddBoolToObject(root, "power_en_vin", (powerstate & ST_POWER_EN_VIN) != 0);
    cJSON_AddBoolToObject(root, "power_en_bat", (powerstate & ST_POWER_EN_BAT) != 0);
    cJSON_AddBoolToObject(root, "power_charge", (powerstate & ST_POWER_CHARGE) != 0);
    cJSON_AddBoolToObject(root, "power_bat_present", (powerstate & ST_POWER_BAT_PRESENT) != 0);
    cJSON_AddBoolToObject(root, "power_bat_error", (powerstate & ST_POWER_BAT_ERROR) != 0);

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        strncpy(buf, json, buf_size - 1);
        buf[buf_size - 1] = '\0';
        free(json);
    } else {
        snprintf(buf, buf_size, "{\"error\":\"json_gen_failed\"}");
    }

    cJSON_Delete(root);
}

void generate_zones_json(char *buf, size_t buf_size) {
    bool zone1_trouble = (inputs_state >> SEND_ALERT_ZONE1) & 1;
    bool zone2_trouble = (inputs_state >> SEND_ALERT_ZONE2) & 1;
    snprintf(buf, buf_size,
        "{"
        "\"zones\":["
        "{\"id\":1,\"trouble\":%s},"
        "{\"id\":2,\"trouble\":%s}"
        "]"
        "}",
        zone1_trouble ? "true" : "false",
        zone2_trouble ? "true" : "false"
    );
}

void generate_valves_json(char *buf, size_t buf_size) {
   // snprintf(buf, buf_size, "{\"closed\":%s}", cfg->close_valve ? "true" : "false");
    bool valve_closed = cfg->close_valve;
    snprintf(buf, buf_size,
        "{"
        "\"valves\":["
        "{\"id\":1,\"closed\":%s}"
        "]"
        "}",
        valve_closed ? "true" : "false"
    );
}


void generate_reg_json(char *buf, size_t buf_size, bool is_valid) {
    if (is_valid){
	  snprintf(buf, buf_size,
        "{"
        "\"valid_data\":%s,"
        "\"public_mqtt\":\"%s\""
        "}",
        "true",
        DEVICE_ID
      );	
	} else {
	  snprintf(buf, buf_size,
        "{"
        "\"valid_data\":%s,"
        "\"public_mqtt\":\"%s\""
        "}",
        "false",
        "null"
      );
	}
    
}

void generate_wifi_json(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    if (!cfg) {
        snprintf(buf, buf_size, "{\"error\":\"cfg_null\"}");
        return;
    }

    if (mac_str[0]== '\0') {
        snprintf(buf, buf_size, "{\"error\":\"mac_empty\"}");
        return;
    }
    snprintf(buf, buf_size,
        "{"
        "\"wifi_mode\":%u,"
        "\"cl_ssid\":\"%s\","
        "\"cl_key\":\"%s\""
        "}",
        cfg->wifi_mode,
        cfg->cl_ssid,
        cfg->cl_key
    );
    update_config=true;
}


void generate_config_json(char *buf, size_t buf_size) {
    
    if (!buf || buf_size == 0) return;


    if (!cfg) {
        snprintf(buf, buf_size, "{\"error\":\"cfg_null\"}");
        return;
    }

    if (mac_str[0]== '\0') {
        snprintf(buf, buf_size, "{\"error\":\"mac_empty\"}");
        return;
    }
   
    snprintf(buf, buf_size,
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
    update_config=true;
}

//I (401615) BLE: Sent upload buff[128]: {"dev":TS7,"vmj":"0","vmn":"1","vadd":23}
void generate_upload_json(char *buf, size_t buf_size) {
	snprintf(fwv,31,"TS%u.%u.%u.%u",TS_TYPE,cfg->vmj,cfg->vmn,cfg->vadd);
    snprintf(buf, buf_size,
        "{"
        "\"dev\":\"%u\","
        "\"vmj\":%u,"
        "\"vmn\":%u,"
        "\"vadd\":%u"
        "}",
        TS_TYPE,cfg->vmj,cfg->vmn,cfg->vadd
    );
}
//-----------------------------------------------------------------------------------------
void handle_command_json(const char *json_str, char *response_buf, size_t buf_size) {
    if (!json_str || !response_buf || buf_size < 32) {
        snprintf(response_buf, buf_size, "{\"error\":\"bad_args\"}");
        return;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        snprintf(response_buf, buf_size, "{\"error\":\"bad_json\"}");
        return;
    }

    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    cJSON *dev = cJSON_GetObjectItem(root, "dev");
    cJSON *target = cJSON_GetObjectItem(root, "target");

    if (!cJSON_IsString(cmd) || !cJSON_IsString(dev)) {
        snprintf(response_buf, buf_size, "{\"error\":\"missing_fields\"}");
        goto cleanup;
    }
    
    if (strcmp(dev->valuestring, DEVICE_ID) != 0 && strcmp(target->valuestring, "regdevice") != 0) {
        snprintf(response_buf, buf_size, "{\"error\":\"dev_mismatch\"}");
        ESP_LOGW("LOCAL RESPONSE", "CURRENT DEVICE NAME IS %s, but %s", DEVICE_ID, dev->valuestring);

        goto cleanup;
    }

    if (strcmp(cmd->valuestring, "get") == 0 && cJSON_IsString(target)) {
        const char *t = target->valuestring;
        if (strcmp(t, "status") == 0) {
            generate_status_json(response_buf, buf_size);
        } else if (strcmp(t, "power") == 0) {
            generate_power_json(response_buf, buf_size);
        } else if (strcmp(t, "zones") == 0) {
            generate_zones_json(response_buf, buf_size);
        } else if (strcmp(t, "valves") == 0) {
            generate_valves_json(response_buf, buf_size);
        } else if (strcmp(t, "wifi") == 0) {
            generate_wifi_json(response_buf, buf_size);
        }  else if (strcmp(t, "config") == 0) {
            generate_config_json(response_buf, buf_size);
        } else if (strcmp(t, "regdevice") == 0) {
			char dev_serial[32];
			snprintf(dev_serial, sizeof(dev_serial),
             "Mastino_%02X%02X%02X%02X%02X%02X",mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3], mac_addr[4], mac_addr[5]);
			if (strcmp(dev->valuestring, dev_serial) == 0){
				generate_reg_json(response_buf, buf_size, true);
			} else {
			    generate_reg_json(response_buf, buf_size, false);
			} 
            
        } else {
            snprintf(response_buf, buf_size, "{\"error\":\"unknown_target\"}");
        }
    } else if (strcmp(cmd->valuestring, "set") == 0 && cJSON_IsString(target)) {
        const char *t = target->valuestring;
        if (strcmp(t, "valve_open") == 0) {
			reset_alarm();
            generate_valves_json(response_buf, buf_size);
        } else if (strcmp(t, "valve_close") == 0) {
			cfg->close_valve |= VST_CLOSE_VALVE_CMD;
            generate_valves_json(response_buf, buf_size);
        } else if (strcmp(t, "config") == 0) {
			cJSON *valueCFG = cJSON_GetObjectItem(root, "value");
			apply_device_config_from_json(valueCFG, false);
			snprintf(response_buf, buf_size, "{\"config\":\"cfg_ok\"}");
        }   else if (strcmp(t, "wifi") == 0) {
 // {"dev":"ESP32_53EF20","cmd":"set","target":"wifi","value":{"wifi_mode":1,"cl_ssid":"HOLDING","cl_key":"0800500507"},"pin":"1234"}
			cJSON *valueCFG = cJSON_GetObjectItem(root, "value");
			apply_device_config_from_json(valueCFG, true);
			snprintf(response_buf, buf_size, "{\"config\":\"wifi_ok\"}");
        } else {
            snprintf(response_buf, buf_size, "{\"error\":\"unknown_target\"}");
        }
    } else {
        snprintf(response_buf, buf_size, "{\"error\":\"unknown_cmd\"}");
    }

cleanup:
    cJSON_Delete(root);
}


