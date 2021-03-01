#ifndef TASK_CHECK_WIFI_H
#define TASK_CHECK_WIFI_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initCheckWiFi();
void checkWiFiTask(void *pvParameter);

#endif