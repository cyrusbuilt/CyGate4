#include "tasks/TaskCheckWiFi.h"

TaskHandle_t initCheckWiFi() {
	TaskHandle_t handle = Application::singleton->wifiCheckTask;
	Application::singleton->initWiFi();
	xTaskCreate(checkWiFiTask, "wifi status check", 2048, NULL, 1, &handle);
	return handle;
}

void checkWiFiTask(void *pvParameter) {
	for (;;) {
		Application::singleton->onCheckWiFi();
		vTaskDelay(CHECK_WIFI_INTERVAL / portTICK_PERIOD_MS);
	}
}