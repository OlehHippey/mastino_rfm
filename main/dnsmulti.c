
/*
 * dnsmulti.c
 * Created on: 27 июн. 2025 г.
 * Author: ProgMen
 */

#include "esp_log.h"
#include "mdns.h"
#include "esp_err.h"
#include "global.h"

void start_mdns_service(const char *hostname, const char *instance_name) {
	char version_str[32];
    snprintf(version_str, sizeof(version_str), "%u.%u.%u", cfg->vmj, cfg->vmn, cfg->vadd);
    
    ESP_ERROR_CHECK(mdns_init());

    // Унікальне hostname, наприклад з MAC
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_ERROR_CHECK(mdns_instance_name_set(instance_name));

    ESP_ERROR_CHECK(mdns_service_add("mastino", "_http", "_tcp", 12345, NULL, 0));

    mdns_txt_item_t txt[] = {
        {"version", version_str},
        {"id", cfg->dev_name},
        {"proto", "udp"}
    };
    ESP_ERROR_CHECK(mdns_service_txt_set("_http", "_tcp", txt, sizeof(txt) / sizeof(txt[0])));
    ESP_LOGW("start_mdns_service", "****************** CURRENT DEVICE MDNS STARTED *******************");
    mdns_srv_started = 2;
}







