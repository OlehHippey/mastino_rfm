/*
 * local_handle.h
 *
 *  Created on: 3 июл. 2025 г.
 *      Author: ProgMen
 */

#ifndef MAIN_LOCAL_HANDLE_H_
#define MAIN_LOCAL_HANDLE_H_


#include <stdio.h>
void generate_status_json(char *buf, size_t buf_size);
void generate_power_json(char *buf, size_t buf_size);
void generate_zones_json(char *buf, size_t buf_size);
void generate_valves_json(char *buf, size_t buf_size);
void generate_config_json(char *buf, size_t buf_size);
void generate_upload_json(char *buf, size_t buf_size);
void handle_command_json(const char *json_str, char *response_buf, size_t buf_size);


#endif /* MAIN_LOCAL_HANDLE_H_ */
