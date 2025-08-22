/*
 * mled.c
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

#include "driver/rmt_tx.h"
#include "global.h"
#include "function.h"
#include "mled.h"
#include "led_strip_encoder.h"
#include <string.h>

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

static const char *TAG = "example";

uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS*3];
sRGB *leds=(sRGB *)led_strip_pixels;

    rmt_channel_handle_t led_chan = NULL;
    rmt_encoder_handle_t led_encoder = NULL;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };


void init_rgb(void)
{
    a_log(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
	if(!mydev) tx_chan_config.gpio_num=RMT_LED_STRIP_GPIO_NUM_OLD;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    a_log(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    rmt_new_led_strip_encoder(&encoder_config, &led_encoder);

    a_log(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
    a_log(TAG, "Start LED rainbow chase");
}

void set_rgb(int num, sRGB *c)
{
    memcpy(led_strip_pixels+(num*3),c,3); 
}
void rgb_send(void)
{
    rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);
    rmt_tx_wait_all_done(led_chan, portMAX_DELAY);    
}

int8_t llmode=-1;
static sRGB colorset={0,0,0};
static uint8_t lto=0, llto=0,chcolor=0,lcolor=0,clrpl=0;

void set_all_color(sRGB *c)
{
    set_rgb(0,c);
    set_rgb(1,c);
    set_rgb(2,c);
// 	if(use_ethernet){
//	sRGB colorset1={0,0,0};
// 		set_rgb(1,&colorset1);
// 		set_rgb(2,&colorset1);
// 	}
    rgb_send();
}
#define CH_C_RED	1
#define CH_C_GREEN	2
#define CH_C_BLUE	4
#define CH_C_WHITE  7

void led_loop_1ms(uint8_t mode)
{
    if(llmode!=mode)
    {
		a_log(TAG,"New mode = %d",mode);
        switch (mode)
        {
			case LED_OFF:
			colorset.R=0;
			colorset.G=0;
			colorset.B=0;
			set_all_color(&colorset);
			lto=0;
			llto=0;
			chcolor=0;
			lcolor=0;
			break;
			case LED_NORM:
			lto=0;
			chcolor=CH_C_WHITE;
			break;
			case LED_BATT:
			lto=0;
			chcolor=CH_C_GREEN;
			break;
			case LED_NOBATT:
			lto=cfg->delfast;
			chcolor=CH_C_WHITE;
			break;
			case LED_CHARGE:
			lto=cfg->delfast;
			chcolor=CH_C_GREEN;
			break;
			case LED_ALARM:
			lto=cfg->delfast;
			chcolor=CH_C_RED;
			break;
			case LED_CLOSEV:
			lto=0;
			chcolor=CH_C_RED;
			break;
			case LED_WIFION:
			lto=0;
			chcolor=CH_C_BLUE;
			break;
			case LED_ADDSENS:
			lto=cfg->delfast;
			chcolor=CH_C_BLUE;
			break;
			case LED_SET_MODE:
			lto=0;
			chcolor=CH_C_BLUE;
			break;
			case LED_SET_VCLOSE_ON:
			lto=0;
			chcolor=CH_C_GREEN;
			break;
			case LED_SET_VCLOSE_OFF:
			lto=cfg->delfast;
			chcolor=CH_C_GREEN;
			break;
			case LED_SET_AROT_ON:
			lto=0;
			chcolor=CH_C_RED;
			break;
			case LED_SET_AROT_OFF:
			lto=cfg->delfast;
			chcolor=CH_C_RED;
			break;
			case LED_SET_FWUPD_ON:
			lto=0;
			chcolor=CH_C_BLUE;
			break;
			case LED_SET_FWUPD_OFF:
			lto=cfg->delfast;
			chcolor=CH_C_BLUE;
			break;
			case LED_SET_NET_CONN:
			lto=cfg->delfast;
			chcolor=CH_C_BLUE;
			break;
			default:
            break;
        }
        llmode=mode;
		if(!lto)
		{
            colorset.R=0;
            colorset.G=0;
            colorset.B=0;
			if(chcolor&1) colorset.R=cfg->maxlight;
			if(chcolor&2) colorset.G=cfg->maxlight;
			if(chcolor&4) colorset.B=cfg->maxlight;
			set_all_color(&colorset);
		}		
    }
    if((llmode)&&(lto))
    {
        if(llto)
        {
            llto--;
        }else
        {
            if(lto)
            {
                if(chcolor)
                {
                    if(clrpl)
                    {
                        if(lcolor<cfg->maxlight)lcolor++;
                        if(lcolor>=cfg->maxlight) clrpl=0;
                    }else
                    {
                        if(lcolor) lcolor--;
                        if(!lcolor) clrpl=1;
                    }
                    colorset.R=0;
                    colorset.G=0;
                    colorset.B=0;
                    if(chcolor&1) colorset.R=lcolor;
                    if(chcolor&2) colorset.G=lcolor;
                    if(chcolor&4) colorset.B=lcolor;
                    set_all_color(&colorset);
                }else
                {
                    colorset.R=0;
                    colorset.G=0;
                    colorset.B=0;
                    set_all_color(&colorset);
                }
                llto=lto;
            }
        }
    }
}



