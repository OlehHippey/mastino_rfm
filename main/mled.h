/*
 * mled.h
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_MLED_H_
#define MAIN_MLED_H_

#include <stdint.h>

typedef struct sRGB_str
{
    uint8_t     G;
    uint8_t     R;
    uint8_t     B;
} sRGB;


void init_rgb(void);
void set_rgb(int num, sRGB *c);
void rgb_send(void);
void led_loop_1ms(uint8_t mode);



#endif /* MAIN_MLED_H_ */
