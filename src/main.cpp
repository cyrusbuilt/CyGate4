/**
 * 
 */

#ifndef ESP32
	#error This firmware is only compatible with ESP32 controllers.
#endif

#include <Arduino.h>
#include "App.h"

Application app;

void initSerial() {
	#ifdef DEBUG
	const bool debug = true;
	#else
	const bool debug = false;
	#endif
    Serial.setDebugOutput(debug);
    Serial.begin(SERIAL_BAUD, SERIAL_8N1);
    Serial.println();
    Serial.println();
    Serial.print(F("INIT: CyGate4 v"));
    Serial.print(FIRMWARE_VERSION);
    Serial.println(F(" booting ..."));
    Serial.println();
}

void setup() {
	initSerial();
    initApplication();
}

void loop() {
    // Nothing to do here. Everything is handled in tasks.
}