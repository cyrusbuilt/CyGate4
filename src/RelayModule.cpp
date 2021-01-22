#include "RelayModule.h"

RelayModule::RelayModule(Adafruit_MCP23017 *busController) {
	this->_busController = busController;
}

uint8_t RelayModule::getRelayAddress(RelaySelect relay) {
	uint8_t result = -1;
	switch (relay) {
		case RelaySelect::RELAY1:
			result = PIN_RM_RELAY1;
			break;
		case RelaySelect::RELAY2:
			result = PIN_RM_RELAY2;
			break;
		case RelaySelect::RELAY3:
			result = PIN_RM_RELAY3;
			break;
		case RelaySelect::RELAY4:
			result = PIN_RM_RELAY4;
			break;
		case RelaySelect::RELAY5:
			result = PIN_RM_RELAY5;
			break;
		default:
			break;
	}

	return result;
}

uint8_t RelayModule::getLedAddressForRelay(RelaySelect relay) {
	uint8_t result = -1;
	switch (relay) {
		case RelaySelect::RELAY1:
			result = PIN_RM_LED1;
			break;
		case RelaySelect::RELAY2:
			result = PIN_RM_LED2;
			break;
		case RelaySelect::RELAY3:
			result = PIN_RM_LED3;
			break;
		case RelaySelect::RELAY4:
			result = PIN_RM_LED4;
			break;
		case RelaySelect::RELAY5:
			result = PIN_RM_LED5;
			break;
		default:
			break;
	}

	return result;
}

bool RelayModule::detect() {
	this->_busController->pinMode(PIN_RM_DET_IN, INPUT);
	this->_busController->pinMode(PIN_RM_DET_OUT, OUTPUT);
	this->_busController->digitalWrite(PIN_RM_DET_OUT, HIGH);
	bool result = this->_busController->digitalRead(PIN_RM_DET_IN) == HIGH;
	this->_busController->digitalWrite(PIN_RM_DET_OUT, LOW);
	return result & (this->_busController->digitalRead(PIN_RM_DET_IN) == LOW);
}

void RelayModule::init() {
	uint8_t relayAddress = 0;
	uint8_t ledAddress = 0;
	for (uint8_t i = 1; i <= 5; i++) {
		relayAddress = this->getRelayAddress((RelaySelect)i);
		this->_busController->pinMode(relayAddress, OUTPUT);
		this->_busController->digitalWrite(relayAddress, LOW);

		ledAddress = this->getRelayAddress((RelaySelect)i);
		this->_busController->pinMode(ledAddress, OUTPUT);
		this->_busController->digitalWrite(ledAddress, LOW);
	}
}

ModuleRelayState RelayModule::getState(RelaySelect relay) {
	uint8_t address = this->getRelayAddress(relay);
	return (ModuleRelayState)this->_busController->digitalRead(address);
}

void RelayModule::setState(RelaySelect relay, ModuleRelayState state) {
	if (this->getState(relay) != state) {
		uint8_t relayAddress = this->getRelayAddress(relay);
		uint8_t ledAddress = this->getLedAddressForRelay(relay);
		this->_busController->digitalWrite(relayAddress, (uint8_t)state);
		this->_busController->digitalWrite(ledAddress, (uint8_t)state);
	}
}

bool RelayModule::isOpen(RelaySelect relay) {
	return this->getState(relay) == ModuleRelayState::OPEN;
}

bool RelayModule::isClosed(RelaySelect relay) {
	return this->getState(relay) == ModuleRelayState::CLOSED;
}

void RelayModule::close(RelaySelect relay) {
	this->setState(relay, ModuleRelayState::CLOSED);
}

void RelayModule::open(RelaySelect relay) {
	this->setState(relay, ModuleRelayState::OPEN);
}

void RelayModule::allRelaysClose() {
	for (uint8_t i = 1; i <= 5; i++) {
		this->close((RelaySelect)i);
	}
}

void RelayModule::allRelaysOpen() {
	for (uint8_t i = 1; i <= 5; i++) {
		this->open((RelaySelect)i);
	}
}