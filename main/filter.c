#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "typedef.h"


static const char *TAG = "window_detect: filter";

void filter_task(void *pvParameter)
{

	info_t * info = (info_t*)pvParameter;

	uint8_t  index;
	sensor_t vRecv;
	filter_t vSend;
	float    sample[CONFIG_N_SAMPLE];
	bool     isReport;

	index    = 0;
	isReport = false;
	while(1)
	{
		if(xQueueReceive(info->recv, &vRecv, 100))
		{

			if (!isReport && index == 4)
			{
				isReport = true;
			}

			vSend.time = vRecv.time;
			sample[index] = vRecv.value;

			index = (index+1) % CONFIG_N_SAMPLE;

			ESP_LOGV(TAG, "%s: %s: %d: R %d",
						__func__,
						pcTaskGetTaskName(NULL),
						xPortGetCoreID(),
						vRecv.value);

			if (isReport)
			{
				vSend.value = 0;
				for (uint8_t i = 0; i < CONFIG_N_SAMPLE; i++) {
					vSend.value += sample[i];
				}

				vSend.value = vSend.value/CONFIG_N_SAMPLE;
				vSend.calibration = vRecv.calibration;

				xQueueSend(info->send, &vSend, 0);

				ESP_LOGD(TAG, "%s: %s: %d: S %f",
							__func__,
							pcTaskGetTaskName(NULL),
							xPortGetCoreID(),
							vSend.value);

			}

		}
	}
}
