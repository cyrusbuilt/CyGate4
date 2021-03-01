#include "tasks/TaskHeartBeat.h"

TaskHandle_t initHeartbeat() {
	TaskHandle_t handle = Application::singleton->heartbeatTask;
	xTaskCreatePinnedToCore(heartBeatTask, "heartbeat", 1024, NULL, 2, &handle, PIO_CORE_ID);
	return handle;
}

void heartBeatTask(void *pvParameter) {
	uint8_t flashes = 0;
	unsigned long delayMs = 0;
	for (;;) {
		switch (Application::singleton->sysState) {
			case SystemState::NORMAL:
				delayMs = 333.33;
				flashes = 3;
				break;
			case SystemState::SYS_DISABLED:
				delayMs = 5000;
				flashes = 2;
				break;
			case SystemState::UPDATING:
				delayMs = 200;
				flashes = 5;
				break;
			case SystemState::BOOTING:
			default:
				break;
		}

		if (flashes == 0 && delayMs == 0) {
			CoreIO.heartbeatLedOn();
			continue;
		}

		for (uint8_t i = 0; i < flashes; i++) {
			vTaskDelay(delayMs / portTICK_PERIOD_MS);
			CoreIO.heartbeatLedOn();
			vTaskDelay(delayMs / portTICK_PERIOD_MS);
			CoreIO.heartbeatLedOff();
		}
	}
}