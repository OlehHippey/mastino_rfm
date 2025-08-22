/*
 * mqttfunc.h
 *
 *  Created on: 10 трав. 2025 р.
 *      Author: voltekel
 home/<home_id>/<room>/<device>/<group>/<item>
 - home_id — унікальний ідентифікатор будинку (може бути user_id, home123, або MAC шлюзу)
 - room — назва кімнати (kitchen, bathroom, garage, hall)
 - device — ID або alias пристрою (наприклад, esp32a1)
 - group — логічне розділення (sensors, actuators, status, control)
 - item — конкретна функція або сенсор (temperature, water_valve, power_socket_1)
 */

#ifndef MAIN_MQTTFUNC_H_
#define MAIN_MQTTFUNC_H_

#include <stdint.h>

void mqttcon(void);
void mqtt_app_start(void);
void power_off(void);
#endif /* MAIN_MQTTFUNC_H_ */
