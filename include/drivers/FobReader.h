#ifndef _FOB_READER_H
#define _FOB_READER_H

#include <Wire.h>

// Commands
#define FOBREADER_DETECT 0xFA
#define FOBREADER_INIT 0xFB
#define FOBREADER_GET_FIRMWARE 0xFC
#define FOBREADER_GET_TAGS 0xFD
#define FOBREADER_GET_AVAILABLE 0xFE
#define FOBREADER_SELF_TEST 0xDC
#define FOBREADER_DETECT_ACK 0xDA
#define FOBREADER_MIFARE_VERSION 0xDB
#define FOBREADER_BAD_CARD 0xDD

// Sizes
#define FOBREADER_FW_PREAMBLE_SIZE 2
#define FOBREADER_MAX_TAG_SIZE 10
#define FOBREADER_TAG_PRESENCE_SIZE 2
#define FOBREADER_TAG_DATA_SIZE 14
#define FOBREADER_MIFARE_VER_SIZE 2
#define FOBREADER_SELF_TEST_SIZE 2

// TODO Init status codes
// TODO Self-test status codes

typedef struct {
	uint8_t records;
	uint8_t tagBytes[FOBREADER_MAX_TAG_SIZE];
	uint8_t size;
	uint8_t id;
} Tag;

class FobReader
{
public:
	void begin(uint8_t addr, TwoWire *theWire = &Wire);
	uint8_t getAddress();
	bool detect();
	uint8_t init();
	bool selfTest();
	String getFirmwareVersion();
	bool isNewTagPresent();
	bool getTagData();
	uint8_t getMiFareVersion();
	void setId(uint8_t id);
	uint8_t getId();
	bool badCard();

	Tag tag;

private:
	void writeByte(uint8_t byte);
	uint8_t readByte();
	uint8_t *readBytes(size_t len);
	uint8_t _i2cAddr;
	uint8_t _id;
	TwoWire *_wire;
};

#endif