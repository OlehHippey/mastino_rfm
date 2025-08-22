/*
 * config.c
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */
#include "cJSON.h"
#include "esp_wifi_types_generic.h"
#include "global.h"
#include "function.h"
#include "nvs.h"
#include <pthread.h>
 #include <stdint.h>
#include <string.h>
#include "esp_log.h"

#define TAG "CONFIG"

volatile uint8_t rr=0;
//----------------------------------------------------------------------------------------------------
uint32_t calc_crccfg(configs *rcfg)
{
	int len=sizeof(configs)-sizeof(uint32_t);
	uint8_t *cc=(uint8_t *)rcfg;
	uint32_t crc=0;
	while (len)
	{
		crc+=*cc++;
		len--;
	}
	crc++;
	return crc;
}


//----------------------------------------------------------------------------------------------------
esp_err_t save_cfg_nvs(configs	*rcfg, char *nsname)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(nsname, NVS_READWRITE, &my_handle);
    if (err) return err;
	rcfg->crc=calc_crccfg(rcfg);
	nvs_set_blob(my_handle, "setting", rcfg, sizeof(configs));
	if (err) goto rerr;
	err = nvs_commit(my_handle);
rerr:
	nvs_close(my_handle);
	return err;


	err = nvs_set_u8(my_handle, "powerst", rcfg->powerst);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "wifi_mode", rcfg->wifi_mode);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "close_valve", rcfg->close_valve);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "zone_state", rcfg->zone_state);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "cstart", rcfg->cstart);
	if (err) goto rerr;
    err = nvs_set_u8(my_handle, "perif_state", rcfg->perifstate);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "lorachannel", rcfg->lorachannel);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "cc1101channel", rcfg->cc1101channel);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "delfast", rcfg->delfast);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "delslow", rcfg->delslow);
	if (err) goto rerr;
	err = nvs_set_u8(my_handle, "maxlight", rcfg->maxlight);
	if (err) goto rerr;
	err = nvs_set_u16(my_handle, "last_state", rcfg->last_state);
	if (err) goto rerr;
	err = nvs_set_u32(my_handle, "setting", rcfg->setting);
	if (err) goto rerr;
	err = nvs_set_u16(my_handle, "vmj", rcfg->vmj);
	if (err) goto rerr;
	err = nvs_set_u16(my_handle, "vmn", rcfg->vmn);
	if (err) goto rerr;
	err = nvs_set_u32(my_handle, "voff", rcfg->voff*10);
	if (err) goto rerr;
	err = nvs_set_u16(my_handle, "vadd", rcfg->vadd);
	if (err) goto rerr;
	err = nvs_set_str(my_handle, "dev_name", rcfg->dev_name);
	if (err) goto rerr;
	err = nvs_set_str(my_handle, "ble_name", rcfg->ble_name);
	if (err) goto rerr;
	err = nvs_set_str(my_handle, "ntp_server", rcfg->ntp_server);
	if (err) goto rerr;
	err = nvs_set_str(my_handle, "cl_ssid", rcfg->cl_ssid);
	if (err) goto rerr;
	err = nvs_set_str(my_handle, "cl_key", rcfg->cl_key);
	if (err) goto rerr;
	err = nvs_commit(my_handle);
//	rerr:
	nvs_close(my_handle);
	return err;
}
//----------------------------------------------------------------------------------------------------
esp_err_t read_cfg_nvs(configs	*rcfg, char *nsname)
{
	memset(rcfg,0,sizeof(configs));
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(nsname, NVS_READWRITE, &my_handle);

    if (err) return err;
	size_t len=sizeof(configs);
	nvs_get_blob(my_handle, "setting", rcfg, &len);
	nvs_close(my_handle);
	if(len!=sizeof(configs)) return 1;
	if(calc_crccfg(rcfg)==rcfg->crc) return 0;
	return 1;

	err = nvs_get_u8(my_handle, "wifi_mode", &rcfg->wifi_mode);
	rr=3;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "close_valve", &rcfg->close_valve);
	rr=4;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "zone_state", &rcfg->zone_state);
	rr=5;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "cstart", &rcfg->cstart);
	rr=6;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "perif_state", &rcfg->perifstate);
	rr=7;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "lorachannel", &rcfg->lorachannel);
	rr=8;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "cc1101channel", &rcfg->cc1101channel);
	rr=9;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "delfast", &rcfg->delfast);
	rr=10;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "delslow", &rcfg->delslow);
	rr=11;
	if (err) goto rerr;
	err = nvs_get_u8(my_handle, "maxlight", &rcfg->maxlight);
	rr=12;
	if (err) goto rerr;
	err = nvs_get_u16(my_handle, "last_state", &rcfg->last_state);
	rr=13;
	if (err) goto rerr;
	err = nvs_get_u32(my_handle, "setting", &rcfg->setting);
	rr=13;
	if (err) goto rerr;
	err = nvs_get_u16(my_handle, "vmj", &rcfg->vmj);
	rr=14;
	if (err) goto rerr;
	err = nvs_get_u16(my_handle, "vmn", &rcfg->vmn);
	rr=15;
	if (err) goto rerr;
	err = nvs_get_u16(my_handle, "vadd", &rcfg->vadd);
	rr=16;
	if (err) goto rerr;
	size_t rl=63;
	err = nvs_get_str(my_handle, "dev_name", rcfg->dev_name,&rl);
	rr=17;
	if (err) goto rerr;
	rl=63;
	err = nvs_get_str(my_handle, "ble_name", rcfg->ble_name,&rl);
	rr=18;
	if (err) goto rerr;
	rl=63;
	err = nvs_get_str(my_handle, "ntp_server", rcfg->ntp_server,&rl);
	rr=19;
	if (err) goto rerr;
	rl=31;
	err = nvs_get_str(my_handle, "cl_ssid", rcfg->cl_ssid,&rl);
	rr=22;
	if (err) goto rerr;
	rl=31;
	err = nvs_get_str(my_handle, "cl_key", rcfg->cl_key,&rl);
	rr=23;
	if (err) goto rerr;
	rl=254;
	uint32_t voff=0;
	err = nvs_get_u32(my_handle, "voff", &voff);
	rr=25;
	if (err) goto rerr;
	rcfg->voff=((float)voff)/10;
// 	uint32_t crc=0;
// 	err = nvs_get_u32(my_handle, "crc", &crc);
// 	if (err) goto rerr;
	nvs_close(my_handle);
	return ESP_OK;
//	if(crc==calc_crccfg(rcfg)) return ESP_OK; else return ESP_FAIL;
rerr:	
	nvs_close(my_handle);
	return err;
}
//----------------------------------------------------------------------------------------------------
void read_config(void)
{
	aalloc(&cfg,sizeof(configs));
	esp_err_t err=read_cfg_nvs(cfg,"config1");
//	print_config();
	if(err==ESP_OK)
	{
		zone1_active=get_Bit(cfg->perifstate, ACTIVE_ZONE1);
 		zone2_active=get_Bit(cfg->perifstate, ACTIVE_ZONE2);
		expend_active=get_Bit(cfg->perifstate, ACTIVE_EXPAND);
		ntp_active=get_Bit(cfg->perifstate, ACTIVE_NTP);
		ota_active=get_Bit(cfg->perifstate, ACTIVE_OTA);
		a_log(TAG, "Read config 1 OK");
		return;
	}
	a_log(TAG, "Read config 1 NOK %d %d",err,rr);
	err=read_cfg_nvs(cfg,"config2");
//	print_config();
	if(err==ESP_OK)
	{
		a_log(TAG, "Read config 2 OK");
		return;
	}
	a_log(TAG, "Read config 2 NOK %d %d",err,rr);
	a_log(TAG, "Read config NOK");
	set_default_config();
    tbeep=LONG_BEEP_KEYPRES;

}

//----------------------------------------------------------------------------------------------------
void  save_config(void)
{
	zone1_active=get_Bit(cfg->perifstate, ACTIVE_ZONE1);
    zone2_active=get_Bit(cfg->perifstate, ACTIVE_ZONE2);
    expend_active=get_Bit(cfg->perifstate, ACTIVE_EXPAND);
    ntp_active=get_Bit(cfg->perifstate, ACTIVE_NTP);
    ota_active=get_Bit(cfg->perifstate, ACTIVE_OTA);
	pthread_mutex_lock(&cfgmutex);
	cfg->last_state&=~ST_SAVE_CONFIG;
	a_log(TAG, "Save config");
	int np=0;
	esp_err_t err=save_cfg_nvs(cfg,"config1");
	while(err)
	{
		if(++np==5)
		{
			ESP_LOGE(TAG, "Save config 1 error %i",err);
			break;
		}
		vTaskDelay(100);
		err=save_cfg_nvs(cfg,"config1");
	}
	np=0;
	err=save_cfg_nvs(cfg,"config2");
	while(err)
	{
		if(++np==5)
		{
			ESP_LOGE(TAG, "Save config 2 error %i",err);
			break;
		}
		vTaskDelay(100);
		err=save_cfg_nvs(cfg,"config2");
	}
	pthread_mutex_unlock(&cfgmutex);
}
//----------------------------------------------------------------------------------------------------
void save_config_t(void *args)
{
	if(cfg) save_config();
	vTaskDelete(NULL);
}
//----------------------------------------------------------------------------------------------------

void set_default_config(void)
{
#ifdef USE_SENSORS
	memset(sensors,0,sizeof(sensor_s)*SENSOR_COUNT);
	save_sensors();
	cfg->lorachannel=1;
	cfg->cc1101channel=3;
#endif
	memset(cfg,0,sizeof(configs));
	cfg->setting=(SET_VCLOSE|SET_FWUPDATE|SET_AROT);
	strcpy(cfg->dev_name,"MASTINO");
#ifdef USE_WIFI	
	strcpy(cfg->cl_ssid,"HOLDING");     
	strcpy(cfg->cl_key,"0800500507");	
	if(mydev==1)
	{
		strcpy(cfg->cl_ssid,"HOLDING");  //MASTNET
		strcpy(cfg->cl_key,"0800500507"); //MastinoNet
		cfg->wifi_mode=WIFI_MODE_STA;
	}else
	if(mydev==2)
	{
		strcpy(cfg->cl_ssid,"HOLDING");   //
		strcpy(cfg->cl_key,"0800500507"); //
		cfg->wifi_mode=WIFI_MODE_STA;
	}else
	{
		strcpy(cfg->cl_ssid,"HOLDING");   //
		strcpy(cfg->cl_key,"0800500507"); //
		cfg->wifi_mode=WIFI_MODE_NULL;
	}
#endif
   	cfg->perifstate=0x1B;

#ifdef USE_BLE
	strcpy(cfg->ble_name,"Mastino");
#endif	
	cfg->voff=10.5;
	cfg->last_state=0;
	cfg->zone_state=0;
	cfg->cstart=0;
	cfg->delslow=LSLOW_TIME;
	cfg->delfast=LFAST_TIME;
	cfg->maxlight=200;
	strcpy(cfg->ntp_server,"0.pool.ntp.org");
	

	save_config();
//	print_config();
}
//----------------------------------------------------------------------------------------------------
#ifdef USE_SENSORS
void set_default_config_sens(void)
{
	memset(sensors,0,sizeof(sensor_s)*SENSOR_COUNT);
	save_sensors();
//	xTaskCreate(save_config_t, "save_config_t", 4096, NULL, 6, NULL);
}
#endif
//----------------------------------------------------------------------------------------------------

void set_default_config_wifi(void)
{
	strcpy(cfg->cl_ssid,"HOLDING");  //MASTNET
	strcpy(cfg->cl_key,"0800500507"); //

	if (mydev==1)
	{
		strcpy(cfg->cl_ssid,"HOLDING");
		strcpy(cfg->cl_key,"0800500507");
		cfg->wifi_mode=WIFI_MODE_STA;
	}else
	if(mydev==2)
	{
		strcpy(cfg->cl_ssid,"HOLDING");
		strcpy(cfg->cl_key,"0800500507");
		cfg->wifi_mode=WIFI_MODE_STA;
	}else
	{
		strcpy(cfg->cl_ssid,"HOLDING");
		strcpy(cfg->cl_key,"0800500507");
		cfg->wifi_mode=WIFI_MODE_NULL;
	}
	xTaskCreate(save_config_t, "save_config_t", 4096, NULL, 6, NULL);
//	print_config();
}
//----------------------------------------------------------------------------------------------------
void apply_device_config_from_json(cJSON *root, bool isWIFI)  //Save configuration
{
    if (!cfg || !root) return;
     pthread_mutex_lock(&cfgmutex); // Захист від одночасного доступу
    cJSON *item = NULL;
     if ((item = cJSON_GetObjectItem(root, "wifi_mode")) && cJSON_IsNumber(item))
        cfg->wifi_mode = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "cl_ssid")) && cJSON_IsString(item))
        strncpy(cfg->cl_ssid, item->valuestring, sizeof(cfg->cl_ssid) - 1);
    if ((item = cJSON_GetObjectItem(root, "cl_key")) && cJSON_IsString(item))
        strncpy(cfg->cl_key, item->valuestring, sizeof(cfg->cl_key) - 1);    
    if (!isWIFI) {
		if ((item = cJSON_GetObjectItem(root, "voff")) && cJSON_IsNumber(item))
        cfg->voff = item->valuedouble;
        if ((item = cJSON_GetObjectItem(root, "dev_name")) && cJSON_IsString(item))
        strncpy(cfg->dev_name, item->valuestring, sizeof(cfg->dev_name) - 1);
        if ((item = cJSON_GetObjectItem(root, "ntp_server")) && cJSON_IsString(item))
        strncpy(cfg->ntp_server, item->valuestring, sizeof(cfg->ntp_server) - 1);
        if ((item = cJSON_GetObjectItem(root, "perif_state")) && cJSON_IsNumber(item))
        cfg->perifstate = item->valueint;
	}    
   
    pthread_mutex_unlock(&cfgmutex);
    save_config(); // Зберегти конфігурацію у NVS
    ESP_LOGI("CONFIG", "Device config applied and saved.");
}
//----------------------------------------------------------------------------------------------------
void print_config(void)
{
	a_log(TAG, "Cfg wifi_mode: %u",cfg->wifi_mode);
	a_log(TAG, "Cfg dev_name : %s",cfg->dev_name);
	a_log(TAG, "Cfg lorachannel: %u",cfg->lorachannel);
	a_log(TAG, "Cfg cc1101channel: %u",cfg->cc1101channel);
	a_log(TAG, "Cfg cl_ssid : %s",cfg->cl_ssid);
	a_log(TAG, "Cfg cl_key : %s",cfg->cl_key);
	a_log(TAG, "Cfg ntp_server : %s",cfg->ntp_server);
	a_log(TAG, "Cfg last_state: %u",cfg->last_state);
	a_log(TAG, "Cfg zone_state: %u",cfg->zone_state);
	a_log(TAG, "Cfg cstart: %u",cfg->cstart);
	a_log(TAG, "Cfg voff: %f",cfg->voff);
	a_log(TAG, "Cfg setting: %u",cfg->setting);
//	ESP_LOGI(TAG, "Calc crc: %lu",calc_crccfg(cfg));
}

 