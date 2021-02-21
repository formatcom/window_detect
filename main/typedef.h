#ifndef _APP_DEF_H_
#define _APP_DEF_H_

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
	QueueHandle_t      recv;
	QueueHandle_t      send;
} info_t;

typedef struct {
	int     off;
	bool invert;
} calibration_t;

typedef struct {
	uint32_t             time;
	int                 value;
	calibration_t calibration;
} sensor_t;

typedef struct {
	uint32_t             time;
	float               value;
	calibration_t calibration;
} filter_t;


typedef struct {
	uint32_t  n_close;
	uint32_t  t0;
	uint32_t  t1;
	uint32_t  tf;
	bool      open;
} window_t;

#endif
