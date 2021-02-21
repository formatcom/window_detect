#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "ota.h"
#include "typedef.h"

FILE * fd;

window_t state_window;
window_t last_state_window;

static const char *TAG = "window_detect: ctrl";

static SemaphoreHandle_t  xSemaphore;
static esp_timer_handle_t cron_report_timer;

static void cron_report_timer_callback(void *args)
{
	xSemaphoreGiveFromISR(xSemaphore, NULL);
}

bool window_is_open(float value, bool invert)
{
	return (value < CONFIG_RANGE_DETECT) ^ invert;
}

void report_state_window(window_t * w, const char * from)
{
	ESP_LOGI(TAG, "%s: n %04d: window is open: %d, t0: %d, t1: %d, tf: %d",
			from,
			w->n_close,
			w->open,
			w->t0,
			w->t1,
			w->tf);

	int err = fprintf(fd, "n: %d, open: %d, t0: %d, t1: %d, tf: %d\n",
							w->n_close,
							w->open,
							w->t0,
							w->t1,
							w->tf);
	if (err < 0)
	{
		ESP_LOGE(TAG, "%s: write dump.log %s", from, strerror(errno));
	}
}

void beat_repost_task(void *pvParameter)
{

	const esp_timer_create_args_t cron_report_timer_args = {
		.callback = &cron_report_timer_callback,
		.name = "cron"
	};

	xSemaphore = xSemaphoreCreateBinary();

	ESP_ERROR_CHECK(esp_timer_create(&cron_report_timer_args, &cron_report_timer));

	// BEAT 1 MIN
	esp_timer_start_periodic(cron_report_timer, 6e+7);

	while(1)
	{
		if(xSemaphoreTake(xSemaphore, 500/portTICK_PERIOD_MS) == pdTRUE )
		{
			if (last_state_window.n_close > 1)
				report_state_window(&last_state_window, __func__);
		}
	}
}

void controller_task(void *pvParameter)
{

	filter_t vRecv;

	QueueHandle_t xQueueSet = (QueueHandle_t)pvParameter;

	state_window.open    = false;
	state_window.t0      =    -1;
	state_window.t1      =    -1;
	state_window.tf      =     0;
	state_window.n_close =     0;

	last_state_window = state_window;

	fd = fopen("/spiflash/dump.log", "at");

	bool init = true;
	bool new_state;

	bool set_calibration = true;

	// MAIN LOOP
	while(1)
	{
		if(xQueueReceive(xQueueSet, &vRecv, 100))
		{

			vRecv.value = fabs(vRecv.value);

			int off = (int)vRecv.calibration.invert + 1;

			if (set_calibration && vRecv.calibration.invert)
			{
				state_window.open    = vRecv.calibration.invert;
				last_state_window    = state_window;

				set_calibration = false;
			}

			state_window.open = window_is_open(vRecv.value, vRecv.calibration.invert);

			new_state = state_window.open != last_state_window.open;

			if (new_state && state_window.open == false)
			{
				state_window.n_close += 1;
			}

			if (new_state && state_window.t1 != -1)
			{
				// reset
				state_window.t0 = state_window.t1;
				state_window.t1 = -1;
			}

			if (state_window.t0 == -1)
			{
				state_window.t0 = xTaskGetTickCount() * portTICK_PERIOD_MS;
			}

			if (new_state && state_window.t1 == -1)
			{
				state_window.t1 = xTaskGetTickCount() * portTICK_PERIOD_MS;
				state_window.tf = state_window.t1 - state_window.t0;
			}

			// valid update firmware
			if (new_state && init == false && state_window.n_close == off)
			{
				if (state_window.tf >= 20000 && state_window.tf <= 60000)
					ota_upgrade();
			}

			if (init && state_window.n_close == 1)
			{
				init = false;
			}

			// report change state and save last state
			if (new_state)
			{
				report_state_window(&state_window, __func__);
			}

			last_state_window = state_window;

		}
	}
}
