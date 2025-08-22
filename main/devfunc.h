/*
 * devfunc.h
 *
 *  Created on: 15 мая 2025 г.
 *      Author: ProgMen
 */

#ifndef MAIN_DEVFUNC_H_
#define MAIN_DEVFUNC_H_


#include <stdbool.h>
#include "global.h"


void chk_mqtt_onesec(void);
void mqtt_send_valve_update(bool closed);

#ifdef USE_SENSORS
void mqtt_publish_sensor_event(uint8_t num);
#endif

void mqtt_client_task(void *pvParameters);


#endif /* MAIN_DEVFUNC_H_ */
