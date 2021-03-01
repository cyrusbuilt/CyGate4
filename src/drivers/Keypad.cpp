#include "drivers/Keypad.h"

void Keypad::begin(uint8_t address, TwoWire *theWire) {
	this->_i2cAddress = address;
	this->_wire = theWire;
	this->_commandData = new KeypadData;

	this->_wire->begin();
}

uint8_t Keypad::getAddress() {
	return this->_i2cAddress;
}

void Keypad::writeByte(uint8_t byte) {
	this->_wire->beginTransmission(this->_i2cAddress);
	this->_wire->write(byte);
	this->_wire->endTransmission();
}

void Keypad::writeBytes(uint8_t *bytes, size_t len) {
	this->_wire->beginTransmission(this->_i2cAddress);
	for (size_t i = 0; i < len; i++) {
		this->_wire->write(bytes[i]);
	}

	this->_wire->endTransmission();
}

uint8_t Keypad::readByte() {
	this->_wire->requestFrom(this->_i2cAddress, (uint8_t)1);
	return this->_wire->read();
}

uint8_t* Keypad::readBytes(size_t len) {
	uint8_t* buffer = new uint8_t; 
	this->_wire->requestFrom(this->_i2cAddress, (uint8_t)len);

	for (size_t i = 0; i < len; i++) {
		buffer[i] = this->_wire->read();
	}

	return buffer;
}

bool Keypad::detect() {
	this->writeByte(KEYPAD_DETECT);
	return this->readByte() == KEYPAD_DETECT_ACK;
}

uint8_t Keypad::init() {
	this->writeByte(KEYPAD_INIT);
	return this->readByte();
}

KeypadData* Keypad::readEntries() {
	// Reset everything
	this->_commandData->size = 0;
	this->_commandData->command = 0;
	for (uint8_t i = 0; i < KEYPAD_DATA_BUFFER_SIZE; i++) {
		this->_commandData->data[i] = 0;
	}

	// Request any awaiting command data.
	this->writeByte(KEYPAD_GET_CMD_DATA);

	// If we get back an ack, then get the command, data len, and
	// command data from the payload.
	uint8_t *payload = this->readBytes(KEYPAD_DATA_BUFFER_SIZE + 3);
	if (payload[0] == KEYPAD_GET_CMD_DATA) {
		this->_commandData->command = payload[1];
		this->_commandData->size = payload[2];
		for (uint8_t i = 0; i < this->_commandData->size; i++) {
			this->_commandData->data[i] = payload[i + 3];
		}

		delete payload;
		return this->_commandData;
	}

	delete payload;
	return nullptr;
}