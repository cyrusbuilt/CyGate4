#include "drivers/CoreIO.h"

CoreIOClass::CoreIOClass() {}

void CoreIOClass::init(Adafruit_MCP23017* controller) {
	this->_controller = controller;
	this->_controller->begin();

	// Init LEDs first.
	this->_heartbeatLED = new LED(PIN_HEARTBEAT_LED, NULL);
	this->_heartbeatLED->init();
	this->_controller->pinMode(PIN_ARM_LED, OUTPUT);
	this->_controller->digitalWrite(PIN_ARM_LED, LOW);

	// Init relay outputs
	for (size_t k = 0; k < sizeof(this->_relayOutputs); k++) {
		this->_controller->pinMode(this->_relayOutputs[k], OUTPUT);
		this->_controller->digitalWrite(this->_relayOutputs[k], LOW);
	}

	// Init opto-isolated zone inputs.
	for (size_t i = 0; i < sizeof(this->_localOptoInputs); i++) {
		pinMode(this->_localOptoInputs[i], INPUT);
		digitalWrite(this->_localOptoInputs[i], HIGH);
	}

	for (size_t j = 0; j < sizeof(this->_expOptoInputs); j++) {
		this->_controller->pinMode(this->_expOptoInputs[j], INPUT);
		this->_controller->pullUp(this->_expOptoInputs[j], HIGH);
	}

	// Init dry-contact zone inputs;
	for (size_t l = 0; l < sizeof(this->_dcInputs); l++) {
		this->_controller->pinMode(this->_dcInputs[l], INPUT);
		this->_controller->pullUp(this->_dcInputs[l], HIGH);
	}

	// Init SPI CS pin
	this->_controller->pinMode(PIN_SPI_CS, OUTPUT);
	this->_controller->digitalWrite(PIN_SPI_CS, HIGH);

	// Init input-only analog pins
	for (size_t m = 0; m < sizeof(this->_inputOnlyAnalogPins); m++) {
		pinMode(this->_inputOnlyAnalogPins[m], INPUT);
		digitalWrite(this->_inputOnlyAnalogPins[m], LOW);
	}

	// TODO default the remaining pins to a particular state?
}

void CoreIOClass::armLedOn() {
	this->_controller->digitalWrite(PIN_ARM_LED, HIGH);
}

void CoreIOClass::armLedOff() {
	this->_controller->digitalWrite(PIN_ARM_LED, LOW);
}

void CoreIOClass::heartbeatLedOn() {
	this->_heartbeatLED->on();
}

void CoreIOClass::heartbeatLedOff() {
	this->_heartbeatLED->off();
}

void CoreIOClass::heartbeatLedFlash(unsigned long delayMs) {
	this->_heartbeatLED->blink(delayMs);
}

uint8_t CoreIOClass::readDryContactZoneInput(OnboardDryContactInput input) {
	return this->_controller->digitalRead(this->_dcInputs[(uint8_t)input]);
}

uint8_t CoreIOClass::readOptoZoneInput(OnboardOptoZoneInput input) {
	uint8_t result = 0;
	switch (input) {
		case OnboardOptoZoneInput::ZONE_1:
		case OnboardOptoZoneInput::ZONE_2:
		case OnboardOptoZoneInput::ZONE_3:
		case OnboardOptoZoneInput::ZONE_4:
		case OnboardOptoZoneInput::ZONE_5:
		case OnboardOptoZoneInput::ZONE_6:
			result = digitalRead(this->_localOptoInputs[(uint8_t)input]);
			break;
		case OnboardOptoZoneInput::ZONE_7:
		case OnboardOptoZoneInput::ZONE_8:
			result = this->_controller->digitalRead(this->_expOptoInputs[(uint8_t)input - 6]);
			break;
		default:
			break;
	}

	return result;
}

void CoreIOClass::relayOn(OnboardRelaySelect relay) {
	uint8_t pin = this->_relayOutputs[(uint8_t)relay];
	if (this->_controller->digitalRead(pin) != HIGH) {
		this->_controller->digitalWrite(pin, HIGH);
	}
}

void CoreIOClass::relayOff(OnboardRelaySelect relay) {
	uint8_t pin = this->_relayOutputs[(uint8_t)relay];
	if (this->_controller->digitalRead(pin) != LOW) {
		this->_controller->digitalWrite(pin, LOW);
	}
}

CoreIOClass CoreIO;