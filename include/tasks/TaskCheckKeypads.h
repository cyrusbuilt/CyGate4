#ifndef TASK_CHECK_KEYPADS_H
#define TASK_CHECK_KEYPADS_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initKeypadDevices();
void keypadTask(void *pvParameter);

#endif