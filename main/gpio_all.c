/*
 * gpio_all.c
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#include "driver/gpio.h"
#include "driver/gptimer_types.h"
#include <pthread.h>
#include <stdio.h>
#include "esp_private/adc_private.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "esp_adc/adc_cali.h"
#include "global.h"
#include "hal/adc_types.h"
#include "driver/adc.h"
// #include "esp_adc/adc_oneshot.h"
// #include "esp_adc/adc_continuous.h"
#include "pins.h"
#include "function.h"
//----------------------------- ADC defines ---------------------------------------------------------
#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type2.data)

#define EXAMPLE_READ_LEN                    256

//---------------------------------------------------

static const char *TAG = "GPIO";

gptimer_handle_t gptimer = NULL;

pthread_mutex_t testm=PTHREAD_MUTEX_INITIALIZER;

//----------------------- ADC variables ---------------------------------------
static adc_channel_t channel[NUM_CH_ADC] = {ADC_CHANNEL_0,ADC_CHANNEL_1,ADC_CHANNEL_2,ADC_CHANNEL_8};
                                            //{ADC_CHANNEL_4,ADC_CHANNEL_5,ADC_CHANNEL_2,ADC_CHANNEL_8}};


uint8_t adc_val1[EXAMPLE_READ_LEN];
uint32_t adcvs1=0;

uint8_t adc_val[EXAMPLE_READ_LEN];
uint32_t adcvs=0;
adc_continuous_handle_t ahandle = NULL;
adc_cali_handle_t adc1_cali_chan0_handle = NULL;
bool do_calibration1_chan0=false;

void timetonext(void)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    if(timetop<8) timeto[timetop++] = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    timeto[timetop]=timeto[0];
}

/*
static void IRAM_ATTR gpio_dat_interrupt_handler(void *args)
{
    iwc++;
    if(gptimer_started)
    {
        gptimer_get_raw_count(gptimer,&dtim_count_up[mux]);
        werror[mux]=0;    
        dat_t_stop();
    }
}
void dat_t_stop(void)
{
    if(gptimer_started)
    {
        gptimer_stop(gptimer);
        gptimer_started=0;
    }
}

void dat_t_start(void)
{
    gptimer_set_raw_count(gptimer,0);
    gptimer_start(gptimer);    
    gptimer_started=1;
}
static bool example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
        gptimer_stop(gptimer);
        gptimer_started=0;
        werror[mux]++;
        werrorch=1;
        return true;
}
esp_err_t gpio_reset_pin1(gpio_num_t gpio_num)
{
    assert(GPIO_IS_VALID_GPIO(gpio_num));
    gpio_config_t cfggpio = {
        .pin_bit_mask = BIT64(gpio_num),
        .mode = GPIO_MODE_INPUT,
        //for powersave reasons, the GPIO should not be floating, select pullup
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfggpio);
    return ESP_OK;
}

*/
esp_err_t gpio_reset_pin_adc(gpio_num_t gpio_num)
{
    assert(GPIO_IS_VALID_GPIO(gpio_num));
    gpio_config_t cfggpio = {
        .pin_bit_mask = BIT64(gpio_num),
        .mode = GPIO_MODE_DISABLE,
        //for powersave reasons, the GPIO should not be floating, select pullup
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfggpio);
    return ESP_OK;
}
esp_err_t gpio_reset_pin_off(gpio_num_t gpio_num)
{
    assert(GPIO_IS_VALID_GPIO(gpio_num));
    gpio_config_t cfggpio = {
        .pin_bit_mask = BIT64(gpio_num),
        .mode = GPIO_MODE_DISABLE,
        //for powersave reasons, the GPIO should not be floating, select pullup
        .pull_up_en = false,
        .pull_down_en = true,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfggpio);
    return ESP_OK;
}
void configure_gpio(void)
{
    gpio_install_isr_service(0);
//  ADC INIT
    gpio_reset_pin_adc(DET_IN1);
    gpio_reset_pin_adc(DET_IN2);
    gpio_reset_pin_adc(VIN_SENS);
#ifdef TS_V7
    gpio_reset_pin_adc(BATT_SENS);
#endif

//    gpio_reset_pin_adc(M1_ADC);
//    gpio_reset_pin_adc(M2_ADC);

// INPUT_INIT
    gpio_reset_pin(VL_STATE);
    gpio_reset_pin(SET_RST);


    gpio_set_direction(VL_STATE, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(VL_STATE, GPIO_PULLUP_ONLY);

#ifdef TS_V7
    gpio_reset_pin(CHRG_ST);
    gpio_set_direction(CHRG_ST, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(CHRG_ST, GPIO_PULLUP_ONLY);
#endif


    gpio_set_direction(SET_RST, GPIO_MODE_DEF_INPUT);
    gpio_set_pull_mode(SET_RST, GPIO_PULLUP_ONLY);



// OUTPUT_INIT

 
    gpio_set_level(BUZZER, 0);    
    gpio_set_direction(BUZZER, GPIO_MODE_DEF_OUTPUT);

    gpio_set_level(VL_NCNO, 0);    
    gpio_set_level(VL_PWR, 0);    
    gpio_set_direction(VL_NCNO, GPIO_MODE_DEF_OUTPUT);
    gpio_set_direction(VL_PWR, GPIO_MODE_DEF_OUTPUT);

#if defined(TS_V7) || defined(TS_V5)
    gpio_set_direction(RALAY_AJAX_IN, GPIO_MODE_DEF_INPUT);
	gpio_set_pull_mode(RALAY_AJAX_IN, GPIO_PULLDOWN_ONLY);
	gpio_set_direction(KONTAKTOR_OUT, GPIO_MODE_DEF_INPUT);
	gpio_set_pull_mode(KONTAKTOR_OUT, GPIO_PULLUP_ONLY);	
#endif
    BUZZER_OFF;

    gpio_set_level(VL_NCNO, 0);    
    gpio_set_level(VL_PWR, 0);    
/*
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // counter will reload with 0 on alarm event
        .alarm_count = 500000, // period = 1s @resolution 1MHz
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb, // register user callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
*/
}
 

void prbuf(void)
{
 pthread_mutex_lock(&testm);
 adcvs=adcvs1;
 memcpy(adc_val,adc_val1,adcvs1);
 pthread_mutex_unlock(&testm);
    a_log(TAG, "ps %lu %u",adcvs,sizeof(adc_digi_output_data_t));
ESP_LOG_BUFFER_HEXDUMP(TAG, adc_val,adcvs,0);  

    for (int i = 0; i < adcvs; i += sizeof(adc_digi_output_data_t)) 
    {
        adc_digi_output_data_t *p = (adc_digi_output_data_t *)(adc_val+i);
        uint32_t *pi=(uint32_t *)p;
        uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
        uint32_t data = EXAMPLE_ADC_GET_DATA(p);
        a_log(TAG, "Ch %lu %lu %08lx",chan_num,data,*pi);
    }

}
 
float raw_to_volt(uint32_t raw)
{
	if (!do_calibration1_chan0) return 0;
    int v=0,r=raw;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, r, &v));
    return (float)v;
}   

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
	adccount++;
    for (int i = 0; i < edata->size; i += sizeof(adc_digi_output_data_t)) 
    {
        adc_digi_output_data_t *p = (adc_digi_output_data_t *)(edata->conv_frame_buffer+i);
        uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
        if(chan_num==8) chan_num=3;
		uint8_t chcount=2;
//		if(inon) chcount=4;
		if(numconv==1) chcount=4;
		if(chan_num<chcount)
		{
			uint32_t data = EXAMPLE_ADC_GET_DATA(p);
			adc_val_avg[chan_num]+=data;
			adc_val_avgcnt[chan_num]++;
		}
    }
    BaseType_t mustYield = pdFALSE;
    uint8_t adcr = QUE_MAIN_ADC;
    edatasize+=edata->size;
    if(que_main) xQueueSendFromISR(que_main, &adcr, &mustYield);
    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 1 * 1000,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };
    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i];// & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        a_log(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        a_log(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        a_log(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}


static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        a_log(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = EXAMPLE_ADC_UNIT,
            .chan = ADC_CHANNEL_0,
            .atten = EXAMPLE_ADC_ATTEN,
            .bitwidth = EXAMPLE_ADC_BIT_WIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        a_log(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = EXAMPLE_ADC_UNIT,
            .atten = EXAMPLE_ADC_ATTEN,
            .bitwidth = EXAMPLE_ADC_BIT_WIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        a_log(TAG, "example_adc_calibration_init - Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}
void adc_init(void)
{
//    s_task_handle = xTaskGetCurrentTaskHandle();

    do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    if (do_calibration1_chan0){
		 a_log(TAG, "adc_init - Calibration Success");
	} else {
		a_log(TAG, "adc_init - Calibration Failue");
	}

    continuous_adc_init(channel, NUM_CH_ADC, &ahandle);
//    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &ahandle);
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(ahandle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(ahandle));
}
void adc_start(void)
{
    if(ahandle) ESP_ERROR_CHECK(adc_continuous_start(ahandle));
}
void adc_stop(void)
{
    if(ahandle) ESP_ERROR_CHECK(adc_continuous_stop(ahandle));
}


void set_kontaktor(uint8_t state)
{
    ESP_LOGE(TAG, "Kontaktor state");
#if defined(TS_V7) || defined(TS_V5)
	if(state)
	{
		gpio_reset_pin(KONTAKTOR_OUT);
		gpio_set_direction(KONTAKTOR_OUT, GPIO_MODE_DEF_OUTPUT);
		gpio_set_level(KONTAKTOR_OUT, 0);
		ESP_LOGE(TAG, "ON Kontaktors");
	}else
	{
		gpio_reset_pin(KONTAKTOR_OUT);
		gpio_set_direction(KONTAKTOR_OUT, GPIO_MODE_DEF_INPUT);
		gpio_set_pull_mode(KONTAKTOR_OUT, GPIO_PULLUP_ONLY);
		ESP_LOGE(TAG, "OFF Kontaktors");
	}

#endif
}