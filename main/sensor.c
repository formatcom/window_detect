#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/adc.h"

#include "typedef.h"

static const char *TAG = "window_detect: hall";

calibration_t sensor_calibrator()
{

	const int n_samples = 250;

	ESP_LOGI(TAG,"%s: Init calibration", __func__);

	int value = 0;
	int i       = 0;

	// wait noise initial
	vTaskDelay(100);

	// retrieve off noise
	value = 0;
	for (i=0; i<n_samples; ++i)
	{
		value += hall_sensor_read();
		vTaskDelay(5);
	}

	value = value/n_samples;

	calibration_t o;

	o.off    = 0 - value;
	o.invert = false;

	if (fabs(value) >= 80)
	{
		o.invert = true;
	}

	ESP_LOGI(TAG, "%s: off calibration %d", __func__, o.off);

	return o;
}

void sensor_task(void *pvParameter)
{

	sensor_t     vSend;

	info_t * info = (info_t*)pvParameter;

	const TickType_t xDelay = 15 / portTICK_PERIOD_MS;


	vSend.calibration = sensor_calibrator();

	while(1)
	{

		vTaskDelay(xDelay);

		vSend.value = hall_sensor_read() + vSend.calibration.off;
		vSend.time  = xTaskGetTickCount() * portTICK_PERIOD_MS;

		ESP_LOGV(TAG,"%s: %s: %d:   %d",
					__func__,
					pcTaskGetTaskName(NULL),
					xPortGetCoreID(),
					vSend.value);

			xQueueSend(info->send, &vSend, 0);
	}
}
