#include "drivers/FobReader.h"

void FobReader::begin(uint8_t address, TwoWire *theWire) {
	this->_i2cAddr = address;
	this->_wire = theWire;
	memset(&this->tag, 0, sizeof(Tag));

	this->_wire->begin();
}

uint8_t FobReader::getAddress() {
	return this->_i2cAddr;
}

void FobReader::writeByte(uint8_t byte) {
	this->_wire->beginTransmission(this->_i2cAddr);
	this->_wire->write(byte);
	this->_wire->endTransmission();
}

uint8_t FobReader::readByte() {
	this->_wire->requestFrom(this->_i2cAddr, (uint8_t)1);
	return this->_wire->read();
}

uint8_t *FobReader::readBytes(size_t len) {
	uint8_t* buffer = new uint8_t;
	this->_wire->requestFrom(this->_i2cAddr, (uint8_t)len);
	for (size_t i = 0; i < len; i++) {
		buffer[i] = this->_wire->read();
	}

	return buffer;
}

bool FobReader::detect() {
	this->writeByte(FOBREADER_DETECT);
	return this->readByte() == FOBREADER_DETECT_ACK;
}

uint8_t FobReader::init() {
	this->writeByte(FOBREADER_INIT);
	return this->readByte();
}

bool FobReader::selfTest() {
	bool result = false;
	this->writeByte(FOBREADER_SELF_TEST);
	
	// Byte 0: 0xDC (command ack)
	// Byte 1: Result (1 = pass, 0 = fail)
	uint8_t *response = this->readBytes(FOBREADER_SELF_TEST_SIZE);
	if (response[0] == FOBREADER_SELF_TEST) {
		result = (bool)response[1];
	}

	delete[] response;
	return result;
}

String FobReader::getFirmwareVersion() {
	String result;
	this->writeByte(FOBREADER_GET_FIRMWARE);

	// Byte 0: 0xFC (command ack)
	// Byte 1: Length of version string in bytes
	// Bytes 2 - n: Version string
	uint8_t *response = this->readBytes(FOBREADER_FW_PREAMBLE_SIZE);
	if (response[0] == FOBREADER_GET_FIRMWARE) {
		size_t len = response[1];

		// The second response is the actual version string in bytes.
		uint8_t *val = this->readBytes(len);
		result = String((char*)val);
		delete val;
	}

	delete[] response;
	return result;
}

bool FobReader::isNewTagPresent() {
	bool result = false;
	this->writeByte(FOBREADER_GET_AVAILABLE);

	// Byte 0: 0xFE (command ack)
	// Byte 1: 1 or 0 (true or false)
	uint8_t *response = this->readBytes(FOBREADER_TAG_PRESENCE_SIZE);
	if (response[0] == FOBREADER_GET_AVAILABLE) {
		result = (bool)response[1];
	}

	delete[] response;
	return result;
}

bool FobReader::getTagData() {
	this->writeByte(FOBREADER_GET_TAGS);

	// Byte 0: 0xFD (command ack)
	// Byte 1: Record count
	// Byte 2: Tag size
	// Byte 3 - 13: Tag UID bytes
	uint8_t *response = this->readBytes(FOBREADER_TAG_DATA_SIZE);
	if (response[0] == FOBREADER_GET_TAGS) {
		memset(&this->tag, 0, sizeof(Tag));
		this->tag.id = this->getId();
		this->tag.records = response[1];
		this->tag.size = response[2];
		for (size_t i = 0; i < this->tag.size; i++) {
			this->tag.tagBytes[i] = response[i + 3];
		}

		delete[] response;
		return true;
	}

	delete[] response;
	return false;
}

uint8_t FobReader::getMiFareVersion() {
	uint8_t result = 0xFF;
	this->writeByte(FOBREADER_MIFARE_VERSION);

	// Byte 0: 0xDB (command ack)
	// Byte 1: The MiFare firmware version code (ie. 0x92)
	uint8_t *response = this->readBytes(FOBREADER_MIFARE_VER_SIZE);
	if (response[0] == FOBREADER_MIFARE_VERSION) {
		result = response[1];
	}

	delete[] response;
	return result;
}

void FobReader::setId(uint8_t id) {
	this->_id = id;
}

uint8_t FobReader::getId() {
	return this->_id;
}

bool FobReader::badCard() {
	this->writeByte(FOBREADER_BAD_CARD);
	return this->readByte() == FOBREADER_BAD_CARD;
}