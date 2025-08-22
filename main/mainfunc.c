/*
 * mainfunc.c
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "global.h"
#include "pins.h"
#include "gpio_all.h"
#include "config.h"
#include "wifi.h"
#include "mled.h"
#include "ext_dev.h"
#include "function.h"
#include "otaflash.h"

#define TAG "main_func"

uint8_t lkey=0;
uint32_t keytime=0;
uint8_t chargestate=0;  //Change status - not init in native code
uint16_t stbeep=0;  //Compare with tbeep
uint16_t tpbeep=0,stpbeep=0;
uint16_t ctbeep=0;
uint16_t chstcountimp=0;   //Check status - not init in native code
uint16_t lchstcountimp=0;  //Check status - not init in native code
uint8_t  leaknum=0;  //Leak num sensor - not init in native code
uint32_t chk_valve_month=0;
uint8_t valve_delay_time=0;
uint8_t tb=0;
int authdev=0;
//---------------------------------------------------------------------------------
void off_input(void)
{
	gpio_reset_pin(DET_IN1);
	gpio_set_direction(DET_IN1, GPIO_MODE_DEF_OUTPUT);
	gpio_set_level(DET_IN1, 0);
	gpio_reset_pin(DET_IN2);
	gpio_set_direction(DET_IN2, GPIO_MODE_DEF_OUTPUT);
	gpio_set_level(DET_IN2, 0);
	
}
//---------------------------------------------------------------------------------
void on_input(void)
{
    gpio_reset_pin_adc(DET_IN1);
    gpio_reset_pin_adc(DET_IN2);	
}
//---------------------------------------------------------------------------------
void chk_led_status(void)
{
	ledstatus=LED_OFF;
	if(cfg->last_state&ST_ONOFF_ON)	
	{
		ledstatus=LED_NORM;
#if defined(TS_V7)

		if(powerstate&ST_POWER_BAT_PRESENT)
		{
			if(powerstate&ST_POWER_CHARGE) ledstatus=LED_CHARGE;
			else
			if(powerstate&ST_POWER_EN_BAT)
			{
				if((powerstate&ST_POWER_EN_VIN)==0) ledstatus=LED_BATT;
			}
			else
			ledstatus=LED_NOBATT;
		}else
		{
			ledstatus=LED_NOBATT;
		}
#endif
		if(cfg->close_valve) ledstatus=LED_CLOSEV;
		if(wifito) ledstatus=LED_WIFION;
#ifdef USE_SENSORS
		if(addsens) ledstatus=LED_ADDSENS;
#endif		
		if(cfg->close_valve&(VST_CLOSE_VALVE_ALERT)) ledstatus=LED_ALARM;
	}
	if(cfg->last_state&ST_SETTING)
	{
		switch(setmode)
		{
			case SET_MODE_VCLOSE:
			if(cfg->setting&SET_VCLOSE) ledstatus=LED_SET_VCLOSE_ON; else ledstatus=LED_SET_VCLOSE_OFF;
			break;
			case SET_MODE_AROT:
			if(cfg->setting&SET_AROT) ledstatus=LED_SET_AROT_ON; else ledstatus=LED_SET_AROT_OFF;
			break;

			case SET_MODE_FWUPDATE:
			if(cfg->setting&SET_FWUPDATE) ledstatus=LED_SET_FWUPD_ON; else ledstatus=LED_SET_FWUPD_OFF;
			break;
			default:
			ledstatus=LED_SET_MODE;
		}
	}
	if (tonet)
	{
		ledstatus=LED_SET_NET_CONN;
	}

}

//---------------------------------------------------------------------------------
void power_on(void)
{
#if defined(TS_V7)
	gpio_set_level(PWR_EN, 1);
#endif
	last_state=0xffff;
	lvalve_state=0xffff;
	lpower_state=0; //0xffff
	lzones_state=0; //
	lwifi_state=0xffff;
	cfg->last_state|=ST_ONOFF_ON;
	ledstatus=LED_NORM;
	a_log(TAG, "Pon");
	tbeep=LONG_BEEP_KEYPRES;
//	cfg->last_state=ST_INIT|ST_SAVE_CONFIG;
#ifdef USE_SENSORS
	if(rf_dev_present) unset_reg_sensor();
#endif
	if (powerstate&ST_POWER_EN_VIN)
	{
		wconn(cfg->wifi_mode);
	}
}
//---------------------------------------------------------------------------------
void power_off(void)
{
	setmode=0;
	setmodetimeout=0;
	cfg->last_state&=~ST_SETTING;
	set_kontaktor(0);
	save_config();
	vTaskDelay(100);
	ledstatus=LED_OFF;
	a_log(TAG, "Poff");
	esp_wifi_stop();
//	tbeep=1000;
	wait_key_off=1;
#ifdef USE_SENSORS
	reset_ext_devs();
#endif
	tbeep=LONG_BEEP_KEYPRES;
#if defined(TS_V7)
	ledstatus=LED_OFF;
	led_loop_1ms(ledstatus);

	uart_wait_tx_idle_polling(0);
	gpio_set_level(PWR_EN, 0);
	vTaskDelay(1000);
	gpio_set_level(BUZZER, 0);
	while(gpio_get_level(PWR_KEY)==0){};
#endif
}
//---------------------------------------------------------------------------------
void reset_alarm(void)
{
	cfg->close_valve&=~(VST_CLOSE_VALVE_ALERT|VST_CLOSE_VALVE_CMD|VST_CLOSE_VALVE_AJAX);
	cfg->zone_state=0;
	a_log(TAG, "SMode open");
#ifdef USE_SENSORS
	reset_alarm_sensors();
#endif
//	chk_led_status();
	cfg->last_state|=ST_SAVE_CONFIG;
	alerts_list[SEND_RESTORED_ALARM]=0xF0; //Restored water leak alarm
	
}

/*-----------------------------------------------------------------------------
| `SET_RST` | `PWR_KEY` | `key_temp` (bit mask)     | Value                    |
| --------- | --------- | ------------------------- | ------------------------ |
| 1         | 1         | 0b00000000                | 0                        |
| 0         | 1         | 0b00000001                | 1 (KEY\_RIGHT)           |
| 1         | 0         | 0b00000010                | 2 (KEY\_LEFT)            |
| 0         | 0         | 0b00000011                | 3 (KEY\_RIGHT+KEY\_LEFT) |
//----------------------------------------------------------------------------*/

void chk_key_1ms(void)
{
	uint8_t key_temp=(gpio_get_level(SET_RST)?0:KEY_RIGHT)|(gpio_get_level(PWR_KEY)?0:KEY_LEFT);
	if(inkey)
	{
		key_temp|=inkey;
		inkey=0;
		keytime=30;
		lkey=key_temp;		
	}
	if(key_temp!=lkey)
	{
		if((!key_temp)&&(!wait_key_off))
		{
			if((keytime>30)&&(keytime<500))
			{
			
				if(lkey==KEY_RIGHT)
				{
					if(cfg->last_state&ST_ONOFF_ON)
					{
						if(cfg->last_state&ST_SETTING)
						{
							setmodetimeout=30;
							if(++setmode==SET_MODE_END) setmode=SET_MODE_VCLOSE;
							a_log(TAG,"Set mode %u",setmode);
							chk_led_status();						
						}else
						{
							cfg->close_valve|=VST_CLOSE_VALVE_CMD;
							a_log(TAG, "SMode close");
						}
					}
				}
				if(lkey==KEY_LEFT)
				{
					if(cfg->last_state&ST_ONOFF_ON)
					{
						if(cfg->last_state&ST_SETTING)
						{
							setmodetimeout=30;
							switch(setmode)
							{
								case SET_MODE_VCLOSE:
								cfg->setting^=SET_VCLOSE;
								a_log(TAG,"Set close valve on batt %s",cfg->setting&SET_VCLOSE?"On":"Off");
								break;
								case SET_MODE_AROT:
								cfg->setting^=SET_AROT;
								a_log(TAG,"Set auto rotate valve on batt %s",cfg->setting&SET_AROT?"On":"Off");
								break;
								case SET_MODE_FWUPDATE:
								cfg->setting^=SET_FWUPDATE;
								a_log(TAG,"Set fw update on network %s",cfg->setting&SET_FWUPDATE?"On":"Off");
								break;
								default:
							}							
							chk_led_status();
						}else
						{
#ifdef USE_SENSORS
							if(addsens)
							{
								addsens=0;
								unset_reg_sensor();
							}
#endif
							if(wifito)
							{
								wifito=0;
								cfg->wifi_mode&=~WIFI_MODE_AP;
								wconn(cfg->wifi_mode);
								chk_led_status();
							}else
							//if((!werror[0])&&(!werror[1]))
							{
								werror[0]=0;
								werror[1]=0;
								reset_alarm();
							}
						}
					}
				}
			            
			}
		}
		lkey=key_temp;
		a_log(TAG, "Key %02x",lkey);
		keytime=0;
	}else
	{
		keytime++;
		if(keytime>30)
		{
			if(lkey)
			{
				if(!wait_key_off)
				{
					if((keytime==31)&&(cfg->last_state&ST_ONOFF_ON)) tbeep=SHORT_BEEP_KEYPRES;
					if(lkey==KEY_LEFT)
					{
						if((cfg->last_state&ST_ONOFF_ON)==0)
						{
							power_on();
						}else
						if(keytime>3000)
						{
							cfg->last_state=ST_ONOFF_OFF;
							power_off();
						}					            
					}else
					if(lkey==KEY_RIGHT)
					{
						if(keytime>3000)
						{
							if(cfg->last_state&ST_SETTING)
							{
								setmodetimeout=30;
								setmode=0;
								cfg->last_state&=~ST_SETTING;
								chk_led_status();
							}else
							{
								if(cfg->close_valve)
								{
#ifdef USE_SENSORS
									set_reg_sensor();
#endif
								}else
								{
									if(!wifito)
									{
										wifito=TIME_WIFI_TO;
										chk_led_status();
										cfg->wifi_mode|=WIFI_MODE_AP;
										wconn(cfg->wifi_mode);
									}else
									{
										wifito=TIME_WIFI_TO;
									}
								}
							}
							wait_key_off=1;
						}
					}else
					if(lkey==(KEY_RIGHT|KEY_LEFT))
					{
						if((cfg->last_state&ST_ONOFF_ON)==0)
						{
							power_on();
							cfg->last_state|=ST_SETTING;
							setmodetimeout=30;
							setmode=SET_MODE_NONE;
							chk_led_status();
							wait_key_off=1;
						}						
						if(keytime>3000)
						{
							if(cfg->last_state&ST_ONOFF_ON)
							{
								if(cfg->close_valve)
								{
#ifdef USE_SENSORS
									if(addsens)
									{
										a_log(TAG, "Set default config sensors");
										set_default_config_sens();
										tbeep=LONG_BEEP_KEYPRES;
										unset_reg_sensor();

									}else
#endif
									{
										a_log(TAG, "Set default config");
										cfg->last_state|=ST_ONOFF_ON;
										set_default_config();
										wconn(cfg->wifi_mode);
										cfg->last_state|=ST_SAVE_CONFIG;
										tbeep=LONG_BEEP_KEYPRES;
										chk_led_status();
									}
								}else
								{
#ifdef USE_WIFI									
									if(wifito)
									{
										a_log(TAG, "Set default config wifi");
										set_default_config_wifi();
										wconn(cfg->wifi_mode);
										tbeep=LONG_BEEP_KEYPRES;
										wifito=0;
										chk_led_status();
									}	
#endif																	
								}
							}
							wait_key_off=1;
						}
					}
				}
			}else
			{
				wait_key_off=0;
			}
		}
	}	
}
//---------------------------------------------------------------------------------
void chk_beep_1ms(void)
{
    if(tbeep)
    {
	    gpio_set_level(BUZZER, 1);
	    if(!tb) a_log(TAG, "BUZZER 1 %u",tbeep);
	    tb=1;
	    tbeep--;
	    if(!tbeep)
	    if(stpbeep) tpbeep=stpbeep;
    }else
    {
	    gpio_set_level(BUZZER, 0);
	    if(tb) a_log(TAG, "BUZZER 0");
	    tb=0;
	    if(tpbeep)
	    {
		    tpbeep--;
		    if(!tpbeep)
		    {
			    if(ctbeep)
			    {
				    if(--ctbeep==0)
				    {
					    stbeep=0;
					    stpbeep=0;
				    }
			    }
			    if(stbeep)tbeep=stbeep;
		    }

	    }
    }
	
}
//---------------------------------------------------------------------------------
void app_1sec(void)
{
	if(authdev) authdev--;
}
//---------------------------------------------------------------------------------
void chk_one_sec(void)
{
    uptime++;
   	if(tonet)
	{
		 if(--tonet==0) chk_led_status();
	}
	if(setmodetimeout)
	{
		if(--setmodetimeout==0)
		{
			cfg->last_state&=~ST_SETTING;
			cfg->last_state|=ST_SAVE_CONFIG;
			chk_led_status();			
		}
	}
#ifdef USE_SENSORS
	if(cfg->last_state&ST_ONOFF_ON)
	if(rf_dev_present)
	{
		if(addsens)
        {
	        if(--addsens==0) unset_reg_sensor();
        }else		
		{
			sensor_chk_one_sec();
		}
	}
#endif	
    if(wifito>0)
    {
	    if(--wifito==0)
	    {
		    cfg->wifi_mode&=~WIFI_MODE_AP;
		    wconn(cfg->wifi_mode);
			chk_led_status();
	    }
    }
	ext_sec_func();
    if(time_power_valve)
    {
        if(--time_power_valve==0)
        {
            OFF(VL_PWR);
            OFF(VL_NCNO);
			if(powertime)
			{
				powertime=0;
				cfg->last_state&=~(ST_ONOFF_ON|ST_ONOFF_PREOFF);
			}
        }
    } else
    {
		if(powertime)
		{
			if(--powertime==0)
			{
				cfg->last_state&=~(ST_ONOFF_ON|ST_ONOFF_PREOFF);
				power_off();
			}
		}
        if(cfg->last_state&ST_ONOFF_PREOFF)
        {
            cfg->last_state&=~(ST_ONOFF_ON|ST_ONOFF_PREOFF);
            power_off();
        }
		if(cfg->last_state&ST_ONOFF_ON)
		{
#if defined(TS_V5)

			if(gpio_get_level(VL_STATE))
#else
#if defined(TS_V7)
		if(!gpio_get_level(VL_STATE))
#else
#error version
#endif
#endif
 
			{
				if((cfg->close_valve&VST_CLOSE_VALVE_ALERT)==0)
				{
					cfg->close_valve|=VST_CLOSE_VALVE_ALERT;
                    alerts_list[SEND_RELAY_POWER]=ALERT_RELAY_POWER;
					tbeep=LONG_BEEP_KEYPRES;
				}
			}			
		}
    }
    if((uptime%2==0)&&(powerstate&ST_POWER_BAT_PRESENT))
    {
        if(chstcountimp>1)
        {
            powerstate&=~ST_POWER_EN_BAT;
			powerstate&=~ST_POWER_CHARGE;
			powerstate|=ST_POWER_BAT_ERROR;
        }else
        {
			powerstate&=~ST_POWER_BAT_ERROR;
            if(chargestate) powerstate|=ST_POWER_CHARGE; else powerstate&=~ST_POWER_CHARGE;
        }
		lchstcountimp=chstcountimp;
       	chstcountimp=0;
	}
    check_power();
#if defined(TS_V7)
    if((cfg->last_state==ST_ONOFF_ON)&&((powerstate&ST_POWER_EN_VIN)==0))
    {
        if(powerstate&ST_POWER_BAT_PRESENT)
        {
            if((vbat>0)&&(vbat<cfg->voff)&&(!powertime))
            {
				cfg->last_state|=ST_ONOFF_OFFVBAT;
                if(cfg->setting&SET_VCLOSE)
				{
					cfg->close_valve|=VST_CLOSE_VALVE_LOWBAT;
					cfg->last_state|=ST_ONOFF_PREOFF;
					cfg->last_state|=ST_SAVE_CONFIG;
				} else
				{
					 a_log(TAG, "Power off on vbat = %f, cfg->voff =  %f, cfg->powerst  %d, powertime =  %d", vbat, cfg->voff,  cfg->powerst, powertime);
					power_off();
				}
            }
        }
    }
#endif
    edatasize=0;
    adccount=0;
	freemem=heap_caps_get_free_size(MALLOC_CAP_8BIT);
#if defined(TS_V5)
    float vbat=0;
#endif            
  //  a_log(TAG, "Uptime %llu, %02x %u %.2f %.2f %u %llu %llu %llu %llu %u %u %u",
  //     uptime,cfg->last_state,vbat,vin,powerstate,adc_val_sr[0],adc_val_sr[1],adc_val_sr[2],adc_val_sr[3],werror[0],werror[1],chargestate,lchrgbat,freemem,adccount);//,cfg->last_state,wifi_state,lkey,chst);

    if(cfg->last_state&ST_ONOFF_ON)
    {

        if((cfg->close_valve&VST_CLOSE_VALVE_ALERT)==0)
        {
            if(ZONE_1_ADC<3000) werror[0]++; else werror[0]=0;
            if(ZONE_2_ADC<3000) werror[1]++; else werror[1]=0;
            if((werror[0]>1)||(werror[1]>1))
            {
	            cfg->close_valve|=VST_CLOSE_VALVE_ALERT;
	            if(werror[0]>1) {
					leaknum=ALERT_ZONE1;
				    alerts_list[SEND_ALERT_ZONE1]=leaknum;
				    inputs_state  |= 1 << SEND_ALERT_ZONE1;
				    update_inputs=true;	
				}
	            if(werror[1]>1) {
				   leaknum=ALERT_ZONE2;
				   alerts_list[SEND_ALERT_ZONE2]=leaknum;
				   inputs_state  |= 1 << SEND_ALERT_ZONE2;
				   update_inputs=true;	
				}
	            tbeep=LONG_BEEP_KEYPRES;
            }
#if defined(TS_V7) || defined(TS_V5)
			if(gpio_get_level(RALAY_AJAX_IN)==1)
			{ //werror - changed 5 to 3 because lenght is 5 and index can be 4 max
				if(werror[3])
				{
					cfg->close_valve|=VST_CLOSE_VALVE_AJAX;
					alerts_list[SEND_RELAY_AJAX]=ALERT_RELAY_POWER;
		            tbeep=LONG_BEEP_KEYPRES;
		        }else
				{
					werror[3]++; 
				}
			}else
			{
				 if(werror[3])
				 {
					 werror[3]--;
				 }else
				 {
					 if(cfg->close_valve&VST_CLOSE_VALVE_AJAX)
					 cfg->close_valve&=~VST_CLOSE_VALVE_AJAX;
				 }
			}
#endif			
        }else
        {
            if(ZONE_1_ADC>3000){
			   werror[0]=0;
			   if ((inputs_state >> SEND_ALERT_ZONE1) & 1){
				   update_inputs=true;	
			       inputs_state  &= ~(1 << SEND_ALERT_ZONE1);	
			   }
			}
            if(ZONE_2_ADC>3000){
				werror[1]=0;
				 werror[0]=0;
			   if ((inputs_state >> SEND_ALERT_ZONE2) & 1){
				   update_inputs=true;	
			       inputs_state  &= ~(1 << SEND_ALERT_ZONE2);	
			   }
			} 
        }
        if((cfg->last_state==ST_ONOFF_ON)&&(!cfg->close_valve))
        {
			if(cfg->setting&SET_AROT)
            if(++chk_valve_month==TIME_AROTATE)
            {
                chk_valve_month=0;
                cfg->close_valve|=VST_CLOSE_VALVE_PERIOD;
            }                
        }else
		{	
			chk_valve_month=0;
		}

        if(cfg->close_valve&VST_CLOSE_VALVE_PERIOD)
        {
            if(++valve_delay_time>=VALVE_DELAY)
            {
                cfg->close_valve&=~VST_CLOSE_VALVE_PERIOD;
                valve_delay_time=0;
            }
        }
#ifdef USE_UDP_CONNECT
		chk_udp_onesec();
#endif

		if(ntp_timeout)
		{
			if(--ntp_timeout==0)
			{
				create_thread("obtain_ntp_time",obtain_ntp_time,NULL);
			}
		}
    }else
	{

#if defined(TS_V7)
		   if((cfg->last_state&ST_ONOFF_PREOFF)==0)
	       {
			  uart_wait_tx_idle_polling(0);
			  gpio_set_level(PWR_EN, 0);
		   }
#endif		
	 }		
	app_1sec();
#ifdef USE_BLE
	lble_1sec();
#endif 
#ifdef USE_UDP
	udpserv_1sec();
#endif
}

//---------------------------------------------------------------------------------
void main_func(uint8_t rb)
{
	if(rb==QUE_MAIN_ADC)
	{
		if(cfg->close_valve&VST_CLOSE_VALVE_ALERT)
		{
			numconv=15;
		}else
		{
			if(++numconv==16)
			{
				on_input();
				numconv=0;
			}
			if(numconv==2) off_input();
		}
	}

#ifdef USE_SENSORS
	if(rb==QUE_MAIN_LORA_ADDSENS)
	{
		set_reg_sensor();
	}
	if(rb==QUE_MAIN_LORA_ENDADDSENS)
	{
		if(addsens)	unset_reg_sensor();
	}
#endif	
    if(rb==QUE_MAIN_UPDATE_FLASH)
    {
	  xTaskCreate(pre_encrypted_ota_task, "pre_encrypted_ota_task", 1024*8, NULL, 6, NULL);
    }
    if(rb==QUE_MAIN_OFFDEVICE)
    {
	    power_off();
    }
    if(rb==QUE_MAIN_TIMER)
    {
	    led_loop_1ms(ledstatus);
#ifdef TS_V7
		if(powerstate&ST_POWER_BAT_PRESENT)
	    {
			chargestate=(gpio_get_level(CHRG_ST)?0:1);
			if(lchst!=chargestate)
			{
				chstcountimp++;
				lchst=chargestate;
			}
		}
#endif		
		chk_key_1ms();
		chk_beep_1ms();
        if(sec)
        {
	        sec=0;
			chk_one_sec();		
		}
		
		if (lpower_state!=powerstate){
			if (!tbeep) tbeep=SHORT_BEEP_KEYPRES;
			a_log(TAG, "Detected Power status changed: new %d vs last %d", powerstate, lpower_state);
			lpower_state=powerstate;
			update_power=true;
		}
		
		if (lzones_state!=cfg->zone_state){
			
			lzones_state=cfg->zone_state;
		    update_sensors=true;	
		}
		
		if (linputs_state!=inputs_state){
			linputs_state=inputs_state;
		    update_inputs=true;	
		}
			
		
        if((last_state!=cfg->last_state)||((lvalve_state!=cfg->close_valve)&&(cfg->last_state&ST_ONOFF_ON)))
        {
		    if (last_state!=cfg->last_state) update_state=true;
            if (lvalve_state!=cfg->close_valve) update_actuators=true;
            
		     update_sensors=true;	
		    xTaskCreate(save_config_t, "save_config_t", 4096, NULL, 6, NULL);
   	        if(cfg->last_state&ST_ONOFF_ON)
	        {
				if(lvalve_state!=cfg->close_valve)
				{
					if(cfg->close_valve)
					{
 						valve_close(cfg->close_valve);
						if(cfg->close_valve&VST_CLOSE_VALVE_ALERT) off_input();
					}else
					{
						valve_open(0);
					}
					lvalve_state=cfg->close_valve;
				}
				chk_led_status();
	        }
	        last_state=cfg->last_state;
        }		
    }

}
