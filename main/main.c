#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "aes.h"
#include "ble_srv.h"
#include "bootloader_common.h"
#include "devfunc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_image_format.h"
//#include "esp_private/bootloader_common.h"  // Для bootloader_common_get_rtc_retain_mem()
//#include "esp_private/rtc_retain_mem.h"     // Для rtc_retain_mem_t


#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"
#include "global.h"
#include "function.h"
#include "mainfunc.h"
#include "mled.h"
#include "nvs_flash.h"
#include "pins.h"
#include "gpio_all.h"
#include "version.h"
#include "otaflash.h"
#include "wifi.h"
#include "ext_dev.h"
#include "mqttfunc.h"


static const char *TAG = "main";
 

typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;


uint16_t mscount=0;

//======================================================================
#ifndef TS5
#endif
void timer_callback(void *param)
{
    if(++mscount==1000)
    {
        mscount=0;
        sec=1;
        ms100=1;
        polsec=1;
    }
    if((mscount%100)==0) ms100=1;
    uint8_t sb=QUE_MAIN_TIMER;
    if(que_main) xQueueSend(que_main, &sb, 0);
}
//----------------------------------------------------------------------
void char_loop(void *arg)
{
	while(1)
	{
		uint8_t value = getchar();
		if(value<255)
		{
			switch(value)
			{
				case '1':
				inkey|=1;
				break;
				case '2':
				inkey|=2;
				break;
			}
			vTaskDelay(5);
		}
	}
}
//----------------------------------------------------------------------
void main_loop(void *arg)
{
    while(1)
    {
        uint8_t rb=0;
        xQueueReceive(que_main, &rb, (TickType_t)portMAX_DELAY);
		if(rb) main_func(rb);
    }
}
//----------------------------------------------------------------------
void led_loop(void *arg)
{
	int cc=0;
	printf("Led loop start");
	sRGB c={0,200,0};
	for (int i=0;i<EXAMPLE_LED_NUMBERS;i++) set_rgb(i,&c);
	rgb_send();
	while(1)
	{
		if(!gpio_get_level(SET_RST))
		{
			if(++cc==5)cc=0;
			sRGB cl={0,0,0};
			switch(cc){
				case 1:
				cl.B=200;
				break;
				case 2:
				cl.G=200;
				break;
				case 3:
				cl.R=200;
				break;
				case 4:
				cl.R=200;
				cl.G=200;
				cl.B=200;
				break;
			}
			for (int i=0;i<EXAMPLE_LED_NUMBERS;i++) set_rgb(i,&cl);
			rgb_send();
			vTaskDelay(500);

		}
		vTaskDelay(5);
	}
}
//----------------------------------------------------------------------
void app_main(void)
{
//    startfs();
#ifdef DEMO_LED

    esp_event_loop_create_default();
	configure_gpio();
    init_rgb();
    sRGB c={0,0,0};
    for (int i=0;i<EXAMPLE_LED_NUMBERS;i++) set_rgb(i,&c);
    xTaskCreate(led_loop, "led_loop", 4096, NULL, 6, NULL);
#else

    rtc_retain_mem_t *rtc_retain_mem = bootloader_common_get_rtc_retain_mem();
	uint8_t power_key=rtc_retain_mem->custom[1];
	freemem=heap_caps_get_free_size(MALLOC_CAP_8BIT);
    mastino_aes_set_key(AES_SET_KEY);
    a_log(TAG, "Mastino dev start");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
	    err = nvs_flash_init();
    }
    esp_event_loop_create_default();
#if defined(TS_V7)
    gpio_set_direction(PWR_EN, GPIO_MODE_DEF_OUTPUT);
//    if(power_key) 
	gpio_set_level(PWR_EN, 1); 
//	else gpio_set_level(PWR_EN, 0);
#ifdef CHRG_CE
	gpio_reset_pin_adc(CHRG_CE);
	gpio_set_direction(CHRG_CE, GPIO_MODE_DEF_OUTPUT);
	CHARGE_DIS
#endif
#endif    
    gpio_reset_pin(PWR_KEY);
    gpio_set_direction(PWR_KEY, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(PWR_KEY, GPIO_PULLUP_ONLY);
    gpio_reset_pin(SET_RST);
    gpio_set_direction(SET_RST, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(SET_RST, GPIO_PULLUP_ONLY);
    read_config();  
#ifdef USE_SENSORS
	read_sensors();
#endif	
	if((cfg->vmj!=get_mj())||(cfg->vmn!=get_mn())||(cfg->vadd!=get_vadd()))
	{
		cfg->vmj=get_mj();
		cfg->vmn=get_mn();
		cfg->vadd=get_vadd();
		a_log(TAG, "Version not match. Firmware updates.");
		accept_ota();
		cfg->last_state|=ST_SAVE_CONFIG;
		tbeep=LONG_BEEP_KEYPRES;
	}
	snprintf(fwv,31,"TS%u.%u.%u.%u",TS_TYPE,cfg->vmj,cfg->vmn,cfg->vadd);
	a_log(TAG, "Version Firmware %s last state %04x",fwv,cfg->last_state);//,PROJECT_NAME);
    if (que_main == NULL) que_main = xQueueCreate(20, 1);
	vTaskDelay(300);	
    adc_init();
#if defined(TS_V7)
    uint8_t rb=0;
    while(rb!=QUE_MAIN_ADC)
    {
		xQueueReceive(que_main, &rb, (TickType_t)portMAX_DELAY);
	}
	check_power();
	if(vbat>6)
	{
		a_logl(ESP_LOG_ERROR,TAG, "Battery present %.2fV",vbat);
#ifdef CHRG_CE
		CHARGE_EN;
#endif
		powerstate|=ST_POWER_BAT_PRESENT;
	}else
	{
		powerstate&=~ST_POWER_BAT_PRESENT;
		cfg->last_state&=~ST_ONOFF_OFFVBAT;
		a_logl(ESP_LOG_ERROR,TAG, "Battery not present");
	}
	a_log(TAG, "Power up vin=%.2fV, vbat=%.2fV ",vin,vbat);
	//TODO: power_key=1;
    if(power_key) 
    {
			if(vin<10)
            {
				if(vbat>10.2)
				{
					power_on();
					if(vbat<10.8)
					{

						powertime=5;
						lvalve_state=cfg->close_valve;
					}else
					{
						cfg->last_state&=~ST_ONOFF_OFFVBAT;
					}
				}else
				{
					cfg->last_state=ST_ONOFF_OFFVBAT;
					power_off();
				}
			}else
			{
				cfg->last_state=ST_ONOFF_OFF;
				power_on();
			}
            a_log(TAG, "Vbat = %.2f",vbat);
            wait_key_off=1;
    } else {
		if(powerstate&ST_POWER_EN_VIN)
		{
			cfg->last_state&=~ST_ONOFF_OFFVBAT;
		}

	}
	cfg->close_valve&=~VST_CLOSE_VALVE_LOWBAT;
#endif            
	uint8_t key_temp=(gpio_get_level(SET_RST)?0:KEY_RIGHT)|(gpio_get_level(PWR_KEY)?0:KEY_LEFT);
	a_log(TAG, "Key on power - %02x ",key_temp);
	if(key_temp==(KEY_LEFT|KEY_RIGHT))
	{
		cfg->last_state|=ST_SETTING;
		setmodetimeout=30;
	}
    ledstatus=LED_OFF;
    configure_gpio();
    #if ESP_IDF_VERSION_MAJOR >= 5
    
    #endif
    esp_netif_init();
	esp_base_mac_addr_get(mac_addr);
	mac64=(uint64_t)mac_addr[0]<<40|(uint64_t)mac_addr[1]<<32|(uint64_t)mac_addr[2]<<24|(uint64_t)mac_addr[3]<<16|(uint64_t)mac_addr[4]<<8|(uint64_t)mac_addr[5];
	sprintf(mac_str,"%02x:%02x:%02x:%02x:%02x:%02x",mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);
	snprintf(DEVICE_ID, sizeof(DEVICE_ID),
             "TS7_ESP32S3_%02X%02X%02X%02X%02X%02X",mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3], mac_addr[4], mac_addr[5]);
// 	for(uint8_t i=0;i<6)
// 	{
// 		mac64|=mac_addr[i];
// 		mac64<<=8;
// 	}
#ifdef USE_RADIO
	//memcpy(gfsk_sync_word_set,mac_addr,6);
#endif
	uint8_t t[6]={0xc0,0x4e,0x30,0x3a,0x04,0xac};
	uint8_t t1[6]={0x24,0x58,0x7c,0xf2,0x4e,0x6c};
	if(!memcmp(t,mac_addr,6)) mydev=1;
	if(!memcmp(t1,mac_addr,6)) mydev=1;
	uint8_t ta[6]={0x64,0xe8,0x33,0x6e,0x4e,0x38};
	uint8_t tm[6]={0x70,0x04,0x1d,0xa8,0x5f,0xc4};
	if(!memcmp(ta,mac_addr,6))	mydev=2;
	if(!memcmp(tm,mac_addr,6))	mydev=3;
	//mydev=1; //TODO: I set 1
	if(mydev==1)
	{
		strcpy(cfg->cl_ssid,"HOLDING");
		strcpy(cfg->cl_key,"0800500507");
		cfg->wifi_mode=WIFI_MODE_STA;
	}
	if(mydev==2)
	{
		strcpy(cfg->cl_ssid,"A4");
		strcpy(cfg->cl_key,"1451070219");
		cfg->wifi_mode=WIFI_MODE_STA;
	}
	if(mydev==3)
	{
		strcpy(cfg->cl_ssid,"Telekom-886394");
		strcpy(cfg->cl_key,"e79p7hmppcdu8h6e");
		cfg->wifi_mode=WIFI_MODE_STA;
		
	}
	print_config();
//    touchpad_init();
    // Prepare and then apply the LEDC PWM timer configuration
  
    wconn(cfg->wifi_mode);
    ble_start();
    const esp_timer_create_args_t my_timer_args = 
    {
      .callback = &timer_callback,
      .name = "My Timer"
    };
    esp_timer_handle_t timer_handler;
    esp_timer_create(&my_timer_args, &timer_handler);
    esp_timer_start_periodic(timer_handler, 1000);
    init_rgb();
    sRGB c={0,0,0};
	for (int i=0;i<EXAMPLE_LED_NUMBERS;i++) set_rgb(i,&c);
//    mux=3;
//    set_mux();
    setenv("TZ", "UTC-2", 1);
    tzset();
//    spi_init();

	init_external();
	mqttcon();
    xTaskCreate(chk_log, "chk_log", 4096, NULL, 6, NULL);
	logrealtime=0;
    xTaskCreate(main_loop, "main_loop", 8198, NULL, 6, NULL);
    xTaskCreate(char_loop, "char_loop", 8198, NULL, 6, NULL);
    xTaskCreate(mqtt_client_task, "mqtt_client_task", 4096, NULL, 6, NULL);

#endif
}
