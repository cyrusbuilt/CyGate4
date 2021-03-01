#ifndef TASK_INPUT_H
#define TASK_INPUT_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initInputTask();
void inputTask(void *pvParameter);

#endif