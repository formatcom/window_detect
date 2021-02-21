#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "typedef.h"
#include "ota.h"
#include "sensor.h"
#include "filter.h"
#include "controller.h"

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

void esp_vfs_fat_init()
{
	const esp_vfs_fat_mount_config_t m_config = {
		.format_if_mount_failed = true,
		.max_files = 4,
		.allocation_unit_size = CONFIG_WL_SECTOR_SIZE
	};

	esp_vfs_fat_spiflash_mount("/spiflash", "storage", &m_config, &s_wl_handle);
}

void app_main(void)
{

	static QueueHandle_t       xQueueSensor;
	static QueueHandle_t       xQueueSet;

	init_ota_config();
	esp_vfs_fat_init();

	xQueueSensor = xQueueCreate(10,  sizeof(sensor_t));
	xQueueSet    = xQueueCreate(20,  sizeof(filter_t));

	static info_t info[2];

	info[0].recv = NULL;
	info[0].send = xQueueSensor;

	xTaskCreatePinnedToCore(&sensor_task,      "Sensor",     8000, &info[0],   0, NULL, 0);

	info[1].recv = xQueueSensor;
	info[1].send = xQueueSet;

	xTaskCreatePinnedToCore(&filter_task,      "Filter",     8000, &info[1],   0, NULL, 0);

	xTaskCreatePinnedToCore(&controller_task,  "Controller", 8000, xQueueSet,  0, NULL, 1);
	xTaskCreatePinnedToCore(&beat_repost_task, "Report",     8000, NULL,       1, NULL, 1);
}
