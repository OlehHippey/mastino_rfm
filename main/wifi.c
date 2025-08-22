/*
 * wifi.c
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_wifi_types_generic.h"
#include "global.h"
#include "function.h"
#include "otaflash.h"


uint8_t wifi_init=0;
static const char *TAG = "WIFI";
static const char *TAG_AP = "WiFi AP";
static const char *TAG_STA = "WiFi Sta";

esp_netif_t *esp_netif_ap = NULL;
esp_netif_t *esp_netif_sta = NULL;


/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_ESP_WIFI_STA_SSID "mywifissid"
*/

/* STA Configuration */


#define EXAMPLE_ESP_WIFI_STA_SSID           "380505833939"
#define EXAMPLE_ESP_WIFI_STA_PASSWD         "a19740216"
#define EXAMPLE_ESP_MAXIMUM_RETRY           2

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WAPI_PSK
#endif

/* AP Configuration */
#define EXAMPLE_ESP_WIFI_CHANNEL            8
#define EXAMPLE_MAX_STA_CONN                10


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
 


static int s_retry_num = 0;



/* FreeRTOS event group to signal when we are connected/disconnected */
//static EventGroupHandle_t s_wifi_event_group;
static uint8_t countconnsta=0;


void makeWiFiconnect(char *TAG_SOURCE){
	strncpy(cfg->cl_ssid, "HOLDING", sizeof(cfg->cl_ssid) - 1);
cfg->cl_ssid[sizeof(cfg->cl_ssid) - 1] = '\0';  // гарантувати завершення

strncpy(cfg->cl_key, "0800500507", sizeof(cfg->cl_key) - 1);
cfg->cl_key[sizeof(cfg->cl_key) - 1] = '\0';

	int con_err = esp_wifi_connect();
	 a_log(TAG_STA, "%s - esp_wifi_connect [ssid: %s  pswrd: %s] result = %d", TAG_SOURCE, cfg->cl_ssid,cfg->cl_key ,con_err);
}

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    a_log(TAG, "Event %lu",event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        a_log(TAG_AP, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        countconnsta++;
        wifito=-1;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        a_log(TAG_AP, "Station "MACSTR" left, AID=%d", MAC2STR(event->mac), event->aid);
        if(countconnsta) countconnsta--;
        if(!countconnsta) wifito=TIME_WIFI_TO;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if(wifi_state&ST_WIFI_STA) makeWiFiconnect("wifi_event_handler: WIFI_EVENT && WIFI_EVENT_STA_START"); // esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_state|=ST_WIFI_STA_CONNECTED;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_state&=~(ST_WIFI_STA_CONNECTED|ST_WIFI_STA_GOT_IP);
        a_log(TAG_STA, "wifi_state %d",wifi_state);
		 makeWiFiconnect("wifi_event_handler: WIFI_EVENT && WIFI_EVENT_STA_DISCONNECTED"); //esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        a_log(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_state|=ST_WIFI_STA_GOT_IP;
        if (mdns_srv_started==0) mdns_srv_started=1;
        if (mdns_udp_started==0) mdns_udp_started=1;
        create_thread("obtain_ntp_time",obtain_ntp_time,NULL);
		xTaskCreate(ota_chk_http_ver, "ota_chk_http_ver", 1024*8, NULL, 6, NULL);
		
    }
}


/* Initialize wifi station */
void wifi_init_sta(void)
{
    if ((cfg->wifi_mode != WIFI_MODE_STA) && (cfg->wifi_mode != WIFI_MODE_APSTA)) return;

    a_log(TAG_STA, "ESP_WIFI_MODE_STA %s %s", cfg->cl_ssid, cfg->cl_key);

    wifi_state |= ST_WIFI_STA;
    wifi_state &= ~(ST_WIFI_STA_CONNECTED | ST_WIFI_STA_GOT_IP);

    if (!esp_netif_sta)
        esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    strcpy((char*)wifi_sta_config.sta.ssid, cfg->cl_ssid);
    strcpy((char*)wifi_sta_config.sta.password, cfg->cl_key);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    a_log(TAG_STA, "wifi_init_sta finished.");
    
    // ❌ НЕ викликаємо esp_wifi_connect() тут — це зробить event handler після start
    // makeWiFiconnect("wifi_init_sta");  ← прибрано
}

static int last_wifi_mode=0;
void wconn(int type)
{
#ifndef USE_WIFI
	return;
#endif
    /* Initialize event group */
//	wifi_init=0;
//    if(cfg->wifi_mode==0) return;

    if(cfg->wifi_mode!=WIFI_MODE_STA){
		cfg->wifi_mode=WIFI_MODE_STA;
	}   
    if(cfg->wifi_mode==WIFI_MODE_NULL)
	{
		last_wifi_mode=0;
		esp_wifi_stop();
	    wifi_state=0;
		return;
	}
//    s_wifi_event_group = xEventGroupCreate();
    /* Register Event handler */
    
    /*Initialize WiFi */
    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    if(!wifi_init)
    {
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);
        esp_wifi_init(&wcfg);
    }
    wifi_init=1;
    esp_wifi_set_mode(cfg->wifi_mode);

	if((last_wifi_mode&WIFI_MODE_AP)&&((cfg->wifi_mode&WIFI_MODE_AP)==0))
	{
		esp_wifi_set_config(WIFI_IF_AP, NULL);
		if(esp_netif_ap) esp_netif_destroy_default_wifi(esp_netif_ap);
		esp_netif_ap=NULL;
	}
    if((cfg->wifi_mode&WIFI_MODE_STA)&&((last_wifi_mode&WIFI_MODE_STA)==0))
	{
		if(!use_ethernet) wifi_init_sta();
	}
	if((last_wifi_mode&WIFI_MODE_STA)&&((cfg->wifi_mode&WIFI_MODE_STA)==0))
	{
		
		esp_wifi_disconnect();
		wifi_state&=~(ST_WIFI_STA_CONNECTED|ST_WIFI_STA_GOT_IP|ST_WIFI_STA);

//		esp_wifi_set_config(WIFI_IF_STA, NULL);
// 		if(esp_netif_sta){
// 			esp_netif_dhcpc_stop(esp_netif_sta);
// 			esp_netif_destroy_default_wifi(esp_netif_sta);
// 		}
//		esp_netif_sta=NULL;
	}
	last_wifi_mode=cfg->wifi_mode;
    a_log(TAG, "wifi start");
    /* Start WiFi */
    int start_wifi_result= esp_wifi_start();
    a_log(TAG, "wifi started [%d] result = %d, state = %d", last_wifi_mode, start_wifi_result, wifi_state);
    if(esp_netif_sta) esp_netif_set_default_netif(esp_netif_sta);
    else 
    if(esp_netif_ap) esp_netif_set_default_netif(esp_netif_ap);
}


