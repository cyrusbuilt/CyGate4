#ifndef TASK_HEARTBEAT_H
#define TASK_HEARTBEAT_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initHeartbeat();
void heartBeatTask(void *pvParameter);

#endif