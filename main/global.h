/*
 * global.h
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_GLOBAL_H_
#define MAIN_GLOBAL_H_

#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>
#include <pthread.h>
#include "config.h"
#include "mqtt_client.h"



#define HOST_MASTINO_SITE    "ts.mastino.in.ua"
#define URL_MASTINO_SITE    "https://ts.mastino.in.ua"
//#define URL_MASTINO_MQTT    "mqtt://s2.bsd.com.ua:7367"
#define URL_MASTINO_MQTT    "mqtt://ts.mastino.in.ua:8883"
//#define HOST_MASTINO_SITE    "95.169.181.96"
//#define URL_MASTINO_SITE    "https://95.169.181.96"
//#define URL_MASTINO_MQTT    "mqtt://95.169.181.96:8883"
#define URL_FIRMWARE        "mastino.bin"
#define DIR_FIRMWARE        "firmware"


#define AES_SET_KEY (uint8_t[]){0x37,0x56,0x82,0x43,0x24,0xbd,0xf8,0x78,0x97,0x13,0x2a,0x87,0x68,0xb9,0xd8,0x7c}

#ifdef USE_BLE
#define BLE_SVC_SPP_UUID16                                  0xBBE0
#define BLE_SVC_SPP_CHR_UUID16                              0xBBE1
#endif

#define NUM_CH_ADC 4

#define TS_TYPE 7

#if (TS_TYPE==73) || (TS_TYPE==75) || (TS_TYPE==52)
#define USE_LORA
#define USE_WIFI
#endif

#if (TS_TYPE==731) || (TS_TYPE==751) || (TS_TYPE==521)
#ifdef USE_LORA
#undef USE_LORA
#endif
#ifdef USE_WIFI
#undef USE_WIFI
#endif
#endif

#if defined(USE_LORA) || defined(USE_CC1101)
#define USE_SENSORS
#endif



#define ZONE_1_ADC adc_val_sr[2]
#define ZONE_2_ADC adc_val_sr[3]
#define NUM_ALERTS 5

#define BLE_ATT_ERR_INSUFFICIENT_AUTHENTICATION 0x05

#define CHK_CLOSE_OPEN 60*60*24*15

#define VALVE_DELAY             30 /* Sec */
#define LEAK_OFFSET             2  /* Sec */

#define BUZZER_ON       {gpio_set_level(BUZZER, 1);}//;gpiostate|=(1<<BUZZER);}
#define BUZZER_OFF      {gpio_set_level(BUZZER, 0);}//;gpiostate&=~(1<<BUZZER);}
#define BUZZER_TGL      {gpio_set_level(BUZZER, gpiostate&(1<<BUZZER)?1:0);gpiostate^=(1<<BUZZER);}


#define CHARGE_EN       {gpio_set_level(CHRG_CE, 0);a_log(TAG, "Charge ENABLE");}
#define CHARGE_DIS      {gpio_set_level(CHRG_CE, 1);a_log(TAG, "Charge DISABLE");}


#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)<(b) ? (a):(b))

#define LSLOW_TIME 20
#define LFAST_TIME 2


#define EXAMPLE_LED_NUMBERS         3
#define RMT_LED_STRIP_GPIO_NUM_OLD  0
#define RMT_LED_STRIP_GPIO_NUM      0


#define TIME_ON_VALVE  30
#define TIME_WIFI_TO   180
#define TIME_AROTATE 60*60*24*15


#define DEFAULT_SCAN_LIST_SIZE 10



//#define DEVICE_ID "esp32a1"
//#define ROOM "mastino"
//#define HOME_ID "home01"

#define SENSOR_COUNT	200
//================================ Types ==================================================

typedef struct alogstr
{
	long mtype;
	int	pid;
	int	flag;
	//	struct timeval tval;
	char tag[32];
	uint64_t	time;
	uint64_t	mac;
	char *str;
} alogs;

typedef struct sensor_str
{
	uint8_t pres;
	uint8_t status;
	uint8_t voltage;
	int8_t  rssi;
	uint64_t last_time;
	uint32_t id[3];
	char    name[32];
}sensor_s;
//================================= Global Flags ==========================================

//----------------------------------------------------------------------------------------------
enum{
	QUE_MAIN_NO=0,
	QUE_MAIN_ADC=1,
	QUE_MAIN_TIMER=2,
#ifdef USE_RADIO	
	QUE_MAIN_RADIO=3,
#endif
#ifdef USE_SENSORS
	QUE_MAIN_RADIO_ADDSENS=4,
	QUE_MAIN_RADIO_ENDADDSENS=5,
#endif
	QUE_MAIN_UPDATE_FLASH=6,
	QUE_MAIN_OFFDEVICE=7
};

//-------------------------------------------------------------------------------------------
enum
{
    LED_OFF             = 0,
    LED_NORM,
    LED_BATT,
    LED_NOBATT,
    LED_CHARGE,
    LED_ALARM,
    LED_CLOSEV,
    LED_WIFION,
    LED_ADDSENS,
    LED_SET_MODE,
    LED_SET_VCLOSE_ON,
    LED_SET_VCLOSE_OFF,
    LED_SET_AROT_ON,
    LED_SET_AROT_OFF,
    LED_SET_FWUPD_ON,
    LED_SET_FWUPD_OFF,
    LED_SET_NET_CONN
};  
//-------------------------------------------------------------------------------------------
enum {
	ST_POWER_NONE=0,
	ST_POWER_EN_VIN=1,
	ST_POWER_EN_BAT=2,
	ST_POWER_CHARGE=4,
	ST_POWER_BAT_PRESENT=8,
	ST_POWER_BAT_ERROR=16
	};
//-------------------------------------------------------------------------------------------	
enum
{
	VST_CLOSE_VALVE_ALERT   = 1,
	VST_CLOSE_VALVE_PERIOD  = 2,
	VST_CLOSE_VALVE_CMD     = 4,
	VST_CLOSE_VALVE_AJAX    = 8,
	VST_CLOSE_VALVE_LOWBAT  = 16,
};	
//-------------------------------------------------------------------------------------------
/*
enum
{
	SET_NONE			= 0,
	SET_VCLOSE			= (1<<0),
	SET_FWUPDATE		= (1<<1),
	SET_NEXT1			= (1<<2),
	SET_NEXT2			= (1<<3),
	SET_NEXT3			= (1<<4),
	SET_NEXT4			= (1<<5),
	SET_NEXT5			= (1<<6),
	SET_NEXT6			= (1<<7),
	SET_NEXT7			= (1<<8)
};
*/
enum
{
	SET_NONE			= 0,
	SET_VCLOSE			= (1<<0),
	SET_AROT			= (1<<1),
	SET_FWUPDATE		= (1<<2),
	SET_NEXT2			= (1<<3),
	SET_NEXT3			= (1<<4),
	SET_NEXT4			= (1<<5),
	SET_NEXT5			= (1<<6),
	SET_NEXT6			= (1<<7),
	SET_NEXT7			= (1<<8)
};


//-------------------------------------------------------------------------------------------
enum
{
	SET_MODE_NONE		= 0,
	SET_MODE_VCLOSE		= 1,
	SET_MODE_AROT		= 2,
	SET_MODE_FWUPDATE	= 3,
	SET_MODE_END		= 4
};
//-------------------------------------------------------------------------------------------	
/*enum
{
    ST_INIT                = 0,
    ST_INIT_OK	           = 1,
    ST_SETTING	           = 2,
    ST_SAVE_CONFIG         = 256
}; */
enum
{
	ST_ONOFF_OFF			= 0,
	ST_ONOFF_ON				= 1,
	ST_ONOFF_PREOFF			= 2,
	ST_ONOFF_OFFVBAT		= 4,
	ST_SETTING	            = 8,
	ST_SAVE_CONFIG          = 128
};
//-------------------------------------------------------------------------------------------
enum 
{
    KEY_RIGHT           = 1,
    KEY_LEFT            = 2
}; 
//-------------------------------------------------------------------------------------------
enum{
	ALERT_NO,
	ALERT_ZONE1,
	ALERT_ZONE2,
	ALERT_ZONE3,
	ALERT_ZONE4,
	ALERT_RELAY_POWER=0x80,
	ALERT_RELAY_AJAX=0x81,
	ALERT_WIRELESS_SENSOR=0x100,
	ALERT_WIRELESS_SENSOR_LOSS=0x200
	};
//-------------------------------------------------------------------------------------------	
enum
{
	HW_ETHER			=	1,
	ETHER_LINK			=	2,
	HW_RADIO				=	4
};
//-------------------------------------------------------------------------------------------


enum{
    ST_WIFI_OFF=0,
    ST_WIFI_AP=1,
    ST_WIFI_STA=2,
    ST_WIFI_STA_CONNECTED=4,
    ST_WIFI_STA_GOT_IP=8

};
//-------------------------------------------------------------------------------------------	
enum
{
	APP_CMD_CHK			= 0,
	APP_CMD_CLOSE		= 1,
	APP_CMD_OPEN		= 2,
	APP_CMD_REGSENS		= 3,
	APP_CMD_REGSENSEND	= 4,
	APP_CMD_LISTSENSOR	= 5,
	APP_CMD_OFFDEVICE	= 6,
	APP_CMD_ETHERSTATE	= 7,
	APP_CMD_FW_UPDATE	= 8
};	
//-------------------------------------------------------------------------------------------	
enum
{
	SEND_ALERT_ZONE1	= 0,
	SEND_ALERT_ZONE2	= 1,
	SEND_RELAY_POWER	= 2,
	SEND_RELAY_AJAX		= 3,
	SEND_RESTORED_ALARM	= 4,
};
//-------------------------------------------------------------------------------------------	
enum
{
	ACTIVE_ZONE1	= 0,
	ACTIVE_ZONE2	= 1,
	ACTIVE_EXPAND	= 2,
	ACTIVE_NTP	= 3,
	ACTIVE_OTA	= 4,
};
//============================== Definations ==================================================
#define SHORT_BEEP_KEYPRES  30
#define LONG_BEEP_KEYPRES  500 

#define TIME_ON_VALVE  30
#define TIME_WIFI_TO   180


#define ON(p) gpio_set_level(p, 1);
#define OFF(p) gpio_set_level(p, 0);

#define LOW  0
#define HIGH 1
//================================= Global Variables ==========================================
extern QueueHandle_t que_main;
extern pthread_mutex_t cfgmutex;
extern configs	*cfg;
extern uint64_t mac64;
extern uint8_t mac_addr[6];
extern char	mac_str[20];
extern char DEVICE_ID[32];
extern uint8_t	mydev;
extern size_t freemem;


extern uint32_t use_ethernet;
extern uint8_t ethernet_pres;
extern uint8_t using_radio;

extern wifi_ap_record_t *scanlist;
extern uint16_t scancount;


extern char fwv[32];


extern uint64_t uptime;   //Order controll in ine_second_timer
extern uint8_t wait_key_off;
extern uint16_t tbeep;

extern uint8_t  sec;   //Second flag
extern uint8_t  ls;
extern uint8_t  polsec; //Half of second
extern uint8_t  ms100;
extern uint32_t intm1c;

extern uint16_t time_power_valve;
extern uint8_t  closev;

extern uint64_t timeto[9];
extern uint8_t timetop;

extern uint8_t setmode;
extern uint8_t setmodetimeout;
extern uint16_t ntp_timeout;

extern uint8_t mdns_srv_started;
extern uint8_t mdns_udp_started;

extern uint16_t  last_state; 
extern uint16_t  lvalve_state;
extern uint8_t  lpower_state;
extern uint8_t  lzones_state;
extern uint8_t  linputs_state;
extern uint8_t lchst;
extern uint16_t  lwifi_state;

extern uint8_t inputs_state;

extern bool update_actuators;
extern bool update_sensors;
extern bool update_inputs;
extern bool update_power;
extern bool update_state;
extern bool update_config;
extern bool restored_alarms;
extern uint16_t alerts_list[NUM_ALERTS];

extern uint8_t powertime;


extern uint8_t ledstatus;
extern int16_t wifito;
extern uint8_t wifi_state;
extern uint8_t powerstate;
extern float vbat;
extern float vin;

extern bool zone1_active;
extern bool zone2_active;
extern bool expend_active;
extern bool ntp_active;
extern bool ota_active;

extern uint64_t adc_val_avg[NUM_CH_ADC];
extern uint64_t adc_val_sr[NUM_CH_ADC];
extern uint64_t adc_val_avgcnt[NUM_CH_ADC];
extern uint8_t numconv;

extern uint32_t edatasize;

extern uint32_t adccount;

extern uint32_t werror[5];

extern uint8_t logrealtime;

extern uint8_t tonet;
extern uint8_t inkey;

extern sensor_s sensors[SENSOR_COUNT];

extern esp_mqtt_client_handle_t clientMQTT;

#endif /* MAIN_GLOBAL_H_ */
