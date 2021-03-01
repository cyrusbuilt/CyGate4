#include "tasks/TaskCheckMqtt.h"

TaskHandle_t initCheckMqtt() {
	TaskHandle_t handle = Application::singleton->mqttCheckTask;
	Application::singleton->initMQTT();
	xTaskCreate(mqttCheckTask, "MQTT status check", 2048, NULL, 1, &handle);
	return handle;
}

void mqttCheckTask(void *pvParameter) {
	for (;;) {
		Application::singleton->onCheckMqtt();
		vTaskDelay(CHECK_MQTT_INTERVAL / portTICK_PERIOD_MS);
	}
}