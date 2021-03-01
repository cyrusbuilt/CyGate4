#include "tasks/TaskSyncClock.h"

TaskHandle_t initClockSync() {
	TaskHandle_t handle = Application::singleton->clockSyncTask;
	Application::singleton->initRTC();
	Application::singleton->initTimeclient();
	xTaskCreate(clockSyncTask, "RTC sync", 2048, NULL, 1, &handle);
	return handle;
}

void printTimestamp(DateTime timestamp) {
	Serial.print(timestamp.month(), DEC);
	Serial.print(F("-"));
	Serial.print(timestamp.day(), DEC);
	Serial.print(F("-"));
	Serial.print(timestamp.year(), DEC);
	Serial.print(F(" "));
	Serial.print(timestamp.hour(), DEC);
	Serial.print(F(":"));
	Serial.print(timestamp.minute(), DEC);
	Serial.print(F(":"));
	Serial.println(timestamp.second(), DEC);
}

void clockSyncTask(void *pvParameter) {
	for (;;) {
		DateTime rtcTime = Application::singleton->rtc.now();
		Serial.print(F("INFO: Current RTC time: "));
		printTimestamp(rtcTime);

		Serial.println(F("INFO: Fetching time from NTP..."));
		while (!Application::singleton->timeClient->update()) {
			Application::singleton->timeClient->forceUpdate();
		}

		Serial.print(F("INFO: NTP time: "));
		DateTime timestamp = DateTime(Application::singleton->timeClient->getEpochTime());
		printTimestamp(timestamp);

		if (rtcTime.unixtime() != timestamp.unixtime()) {
			uint32_t diff = timestamp.unixtime() - rtcTime.unixtime();
			Serial.print(F("INFO: Time differential: "));
			Serial.print(diff, DEC);
			Serial.print(F(" seconds"));
			Serial.println(F("INFO: Adjusting RTC time..."));
			Application::singleton->rtc.adjust(timestamp);
		}
		else {
			Serial.println(F("INFO: Skipping time sync. Not required."));
		}

		vTaskDelay(CLOCK_SYNC_INTERVAL / portTICK_PERIOD_MS);
	}
}