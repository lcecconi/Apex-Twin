#ifndef APEX_TASKS_H
#define APEX_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>

typedef struct {
	uint32_t utc_time_ms;
	double latitude;
	double longitude;
	float altitude_m;
	uint8_t gps_mode;
	float correction_age_s;
	char nmea_sentence[256];
} gps_data_t;

extern QueueHandle_t gps_data_queue;

void gps_setup_task(void *pvParameters);
void dummy_task(void *pvParameters);
void gps_monitor_task(void *pvParameters);
void ntrip_task(void *pvParameters);
#endif