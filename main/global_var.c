/*
 * global_var.c
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "global.h"
#include "mqtt_client.h"



QueueHandle_t que_main = NULL;
pthread_mutex_t cfgmutex=PTHREAD_MUTEX_INITIALIZER;
configs	*cfg=NULL;
uint64_t mac64;
uint8_t mac_addr[6]={};
char	mac_str[20]={0};
char DEVICE_ID[32]={0};
uint8_t	mydev=0;
size_t freemem=0;

wifi_ap_record_t *scanlist=NULL;
uint16_t scancount=0;

uint32_t use_ethernet=0;
uint8_t ethernet_pres=0;
uint8_t using_radio=0;

char fwv[32]={0};

uint64_t uptime=0; //TODO: Not understandable variable without any initialisation in native code
uint8_t wait_key_off=0;   //Wait power off
uint16_t tbeep=0; 

uint8_t  sec=0;   //Second flag
uint8_t  ls=0;
uint8_t  polsec=0; //Half of second
uint8_t  ms100=0; 
uint32_t intm1c=0;


uint8_t setmode=0;
uint8_t setmodetimeout=0;
uint16_t ntp_timeout=0;

uint64_t timeto[9]={0};
uint8_t timetop=0;

uint16_t time_power_valve=0;
uint8_t  closev=0;

uint8_t mdns_srv_started=0;
uint8_t mdns_udp_started=0;

uint16_t  last_state=0xffff; 
uint16_t  lvalve_state=0xffff;
uint8_t  lpower_state=0;
uint8_t  lzones_state=0;
uint8_t  linputs_state=0;
uint8_t lchst=1;  //Last change status  - not init in native code
uint16_t  lwifi_state=0xffff;

uint8_t  inputs_state=0;

bool update_actuators=true;
bool update_sensors=true;
bool update_inputs=true;
bool update_power=true;
bool update_state=true;
bool update_config=true;
uint16_t alerts_list[NUM_ALERTS]={0};

uint8_t powertime=0;


uint8_t ledstatus=0;
int16_t wifito=0;
uint8_t wifi_state=0;
uint8_t powerstate=0;
float vbat=0;
float vin=0;

bool zone1_active=true;
bool zone2_active=true;
bool expend_active=true;
bool ntp_active=true;
bool ota_active=true;

uint64_t adc_val_avg[NUM_CH_ADC]={0};
uint64_t adc_val_sr[NUM_CH_ADC]={0};
uint64_t adc_val_avgcnt[NUM_CH_ADC]={0};
uint8_t numconv=0;

uint32_t edatasize=0;

uint32_t adccount=0;

uint32_t werror[5]={0,0,0,0,0};

uint8_t logrealtime=1;


uint8_t tonet=0;
uint8_t inkey=0;
sensor_s    sensors[SENSOR_COUNT]={};

esp_mqtt_client_handle_t clientMQTT = NULL;
