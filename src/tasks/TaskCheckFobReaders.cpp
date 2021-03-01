#include "tasks/TaskCheckFobReaders.h"

TaskHandle_t initFobReaderDevices() {
	TaskHandle_t handle = Application::singleton->fobReaderCheckTask;
	Application::singleton->initFobReaders();
	xTaskCreate(fobReaderTask, "fob reader subsystem", 2048, NULL, 2, &handle);
	return handle;
}

void fobReaderTask(void *pvParameter) {
	QueueHandle_t queue = Application::singleton->fobReaderQueue;
	for (;;) {
		for (size_t i = 0; i < Application::singleton->fobReaders.size(); i++) {
			auto fr = Application::singleton->fobReaders.at(i);
			if (fr.isNewTagPresent() && fr.getTagData()) {
				xQueueSend(queue, &fr.tag, 0);
			}
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}