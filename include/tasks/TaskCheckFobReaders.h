#ifndef TASK_CHECK_FOB_READERS_H
#define TASK_CHECK_FOB_READERS_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initFobReaderDevices();
void fobReaderTask(void *pvParameter);

#endif