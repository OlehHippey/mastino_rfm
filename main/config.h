/*
 * config.h
 *
 *  Created on: 3 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>
 

typedef struct config_str
{
	uint8_t     powerst;
	uint8_t     wifi_mode;
	uint16_t    last_state;
	uint8_t     close_valve;
	uint8_t     zone_state;
	uint8_t     cstart;
	uint8_t     perifstate;
	uint8_t     lorachannel;
	uint8_t     cc1101channel;
	char        dev_name[64];
	char        ble_name[64];
	char        cl_ssid[32];
	char        cl_key[32];
	char        ntp_server[64];
	float       voff;
	uint8_t		delfast;
	uint32_t	setting;
	uint8_t		delslow;
	uint8_t		maxlight;
	uint16_t	vmj;
	uint16_t	vmn;
	uint16_t	vadd;
    uint32_t	crc;
}configs;

void  save_config(void);
void read_config(void);
void  set_default_config(void);
void save_config_t(void *args);
void set_default_config_wifi(void);
void apply_device_config_from_json(cJSON *root, bool isWIFI);
void print_config(void);

#endif /* MAIN_CONFIG_H_ */
