/*
 * gpio_all.h
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_GPIO_ALL_H_
#define MAIN_GPIO_ALL_H_

#include "esp_err.h"
#include "global.h"
#include "soc/gpio_num.h"

void configure_gpio(void);
esp_err_t gpio_reset_pin_adc(gpio_num_t gpio_num);
float raw_to_volt(uint32_t raw);
void adc_init(void);
void set_kontaktor(uint8_t state);

#endif /* MAIN_GPIO_ALL_H_ */
