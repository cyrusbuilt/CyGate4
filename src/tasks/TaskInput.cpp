#include "tasks/TaskInput.h"

TaskHandle_t initInputTask() {
	TaskHandle_t handle = Application::singleton->inputTask;
	xTaskCreatePinnedToCore(inputTask, "zone inputs", 1024, NULL, 2, &handle, PIO_CORE_ID);
	return handle;
}

void inputTask(void *pvParameter) {
	QueueHandle_t optoQueue = Application::singleton->optoContactQueue;
	QueueHandle_t dcQueue = Application::singleton->dryContactQueue;

	uint8_t value = 0;
	for (;;) {
		for (uint8_t i = 0; i < 8; i++) {
			value = CoreIO.readOptoZoneInput((OnboardOptoZoneInput)i);
			xQueueSend(optoQueue, &value, 0);

			value = CoreIO.readDryContactZoneInput((OnboardDryContactInput)i);
			xQueueSend(dcQueue, &value, 0);
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}