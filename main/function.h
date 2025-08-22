/*
 * function.h
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_FUNCTION_H_
#define MAIN_FUNCTION_H_

#include "esp_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define BitSet(arg,posn) ((arg) | (1L << (posn)))
#define BitClr(arg,posn) ((arg) & ~(1L << (posn)))
#define BitTst(arg,posn) BOOL((arg) & (1L << (posn)))
#define BitFlp(arg,posn) ((arg) ^ (1L << (posn)))


void check_power(void);
void obtain_ntp_time(void);
void create_thread(char *name,void *funtr,void *data);
void valve_close(uint8_t num);
void valve_open(uint8_t num);
int aalloc(void *al,size_t size);
void chk_log(void *arg);
void a_log(const char *type, char *fmt,...) ;
void a_logl(esp_log_level_t level, const char *type, char *fmt,...) ;
uint64_t gettimesec(void);
uint32_t hostname_to_ip(char *hostname);

unsigned char set_Bit(char num_val, char num_bit);
unsigned char clr_Bit(char num_val, char num_bit);
unsigned char tgl_Bit(char num_val, char num_bit);
bool get_Bit(char num_val, char num_bit);

#endif /* MAIN_FUNCTION_H_ */
