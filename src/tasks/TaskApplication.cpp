#include "tasks/TaskApplication.h"

void initApplication() {
	TaskHandle_t taskHandle = Application::singleton->applicationTask;
	xTaskCreate(ApplicationTask, "main program", 8192, NULL, 2, &taskHandle);
}

void ApplicationTask(void *pvParameter) {
	Application::singleton->init();
	for (;;) {
		Application::singleton->update();
	}
}