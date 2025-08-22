/*
 * otaflash.h
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

#ifndef MAIN_OTAFLASH_H_
#define MAIN_OTAFLASH_H_

void accept_ota(void);
void pre_encrypted_ota_task(void *pvParameter);
void ota_chk_http_ver(void *pvParameter);

#endif /* MAIN_OTAFLASH_H_ */
