#ifndef TASK_CHECK_MQTT_H
#define TASK_CHECK_MQTT_H

#include <Arduino.h>
#include "App.h"

TaskHandle_t initCheckMqtt();
void mqttCheckTask(void *pvParameter);

#endif