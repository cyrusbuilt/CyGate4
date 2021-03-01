#ifndef TASK_SYNC_CLOCK_H
#define TASK_SYNC_CLOCK_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initClockSync();
void clockSyncTask(void *pvParameter);

#endif