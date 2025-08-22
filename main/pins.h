/*
 * pins.h
 *
 * Created: 20.12.2024 13:26:14
 *  Author: Aleks
 */ 


#ifndef PINS_H_
#define PINS_H_


#include "types.h"

#if (TS_TYPE==73) || (TS_TYPE==731) || (TS_TYPE==75) || (TS_TYPE==751)
#define TS_V7
#endif

#if (TS_TYPE==51) || (TS_TYPE==511) || (TS_TYPE==52) || (TS_TYPE==521)
#ifdef TS_V7
#undef TS_V7
#endif
#define TS_V5
#endif


#if (TS_TYPE==75) || (TS_TYPE==751)
#define EXT_SLOTS	1

#define PWR_EN      8
#define BUZZER      46
#define VL_NCNO     36
#define VL_STATE    38
#define VIN_SENS    1
#define BATT_SENS   2
#define DET_IN1     3
#define DET_IN2     9
#define SET_RST     41
#define CHRG_ST     40
#define PWR_KEY     39
#define RALAY_AJAX_IN 45
#define KONTAKTOR_OUT 37

#define VL_PWR      35
#define CHRG_CE     42

#define EXTIO_M2_40PIN 48
#define EXTIO_M2_CS_PIN 47
#define EXTIO_M2_INT_PIN 21
#define EXTIO_M2_RST_PIN 14
#if EXT_SLOTS > 1
#define EXTIO_M1_40PIN 48
#define EXTIO_M1_CS_PIN 47
#define EXTIO_M1_INT_PIN 21
#define EXTIO_M1_RST_PIN 14
#endif
#endif

#if (TS_TYPE==73) || (TS_TYPE==731)
#define EXT_SLOTS	2
#define PWR_EN      7
#define BUZZER      46
#define VL_NCNO     36
#define VL_STATE    37
#define VIN_SENS    1
#define BATT_SENS   2
#define DET_IN1     3
#define DET_IN2     9
#define SET_RST     39
#define CHRG_ST     8
#define PWR_KEY     42
#define RALAY_AJAX_IN 40
#define KONTAKTOR_OUT 41

#define VL_PWR      45
#define CHRG_CE		6

#define EXTIO_M1_40PIN 4
#define EXTIO_M1_CS_PIN 10
#define EXTIO_M1_INT_PIN 14
#define EXTIO_M1_RST_PIN 21
#if EXT_SLOTS > 1
#define EXTIO_M2_40PIN 38
#define EXTIO_M2_CS_PIN 47
#define EXTIO_M2_INT_PIN 19
#define EXTIO_M2_RST_PIN 20
#endif
#endif

#if (TS_TYPE==93) || (TS_TYPE==931)
#define EXT_SLOTS	2
#define PWR_EN      8
#define BUZZER      46
#define VL_NCNO     36
#define VL_STATE    38
#define VIN_SENS    1
#define BATT_SENS   2
#define DET_IN1     3
#define DET_IN2     9
#define SET_RST     41
#define CHRG_ST     40
#define PWR_KEY     39

#define VL_PWR      35
#define CHRG_CE     4
#define EXTIO_M1_40PIN 41
#define EXTIO_M1_CS_PIN 10
#define EXTIO_M1_INT_PIN 14
#define EXTIO_M1_RST_PIN 21
#if EXT_SLOTS > 1
#define EXTIO_M2_40PIN 40
#define EXTIO_M2_CS_PIN 47
#define EXTIO_M2_INT_PIN 19
#define EXTIO_M2_RST_PIN 20
#endif
#endif

#if (TS_TYPE==51) || (TS_TYPE==511) || (TS_TYPE==52) || (TS_TYPE==521)
#define EXT_SLOTS	2
#define BUZZER      46
#define VL_NCNO     36
#define VL_STATE    37
#define VIN_SENS    1
#define DET_IN1     3
#define DET_IN2     9
#define SET_RST     39
#define PWR_KEY     42
#define RALAY_AJAX_IN 40
#define KONTAKTOR_OUT 41

#define VL_PWR      45

#define EXTIO_M1_40PIN 4
#define EXTIO_M1_CS_PIN 10
#define EXTIO_M1_INT_PIN 14
#define EXTIO_M1_RST_PIN 21
#if EXT_SLOTS > 1
#define EXTIO_M2_CS_PIN 47
#define EXTIO_M2_40PIN 38
#define EXTIO_M2_INT_PIN 19
#define EXTIO_M2_RST_PIN 20
#endif
#endif



#define CONFIG_SCK_GPIO 12
#define CONFIG_MOSI_GPIO 11
#define CONFIG_MISO_GPIO 13


#endif /* PINS_H_ */