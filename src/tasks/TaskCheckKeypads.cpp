#include "tasks/TaskCheckKeypads.h"

TaskHandle_t initKeypadDevices() {
	TaskHandle_t handle = Application::singleton->keypadCheckTask;
	Application::singleton->initKeypads();
	xTaskCreate(keypadTask, "keypad subsystem", 2048, NULL, 2, &handle);
	return handle;
}

void keypadTask(void *pvParameter) {
	QueueHandle_t queue = Application::singleton->keypadQueue;
	for (;;) {
		for (size_t i = 0; Application::singleton->keypads.size(); i++) {
			auto kp = Application::singleton->keypads.at(i);
			auto data = kp.readEntries();
			if (data != nullptr) {
				xQueueSend(queue, &data, 0);
			}
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}