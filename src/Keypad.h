#ifndef _KEYPAD_H
#define _KEYPAD_H

#include <Wire.h>

// Commands
#define KEYPAD_INIT 0xDA
#define KEYPAD_DETECT 0xDB
#define KEYPAD_DETECT_ACK 0xDD
#define KEYPAD_GET_CMD_DATA 0xDE

// Sizes
#define KEYPAD_DATA_BUFFER_SIZE 4

// TODO Init status codes

struct KeypadData {
	uint8_t size;
	uint8_t command;
	uint8_t data[KEYPAD_DATA_BUFFER_SIZE];
};

enum class KeypadCommands : uint8_t {
	DISARM = 0x01,
	ARM_STAY = 0x02,
	ARM_AWAY = 0x03,
	UNLOCK = 0x04,
	LOCK = 0x05
};

class Keypad
{
public:
	void begin(uint8_t address, TwoWire *theWire = &Wire);
	uint8_t getAddress();
	bool detect();
	uint8_t init();
	void loop();
	void setCommandHandler(void (*onCommandDataReceived)(KeypadData* cmdData));

private:
	void writeByte(uint8_t byte);
	void writeBytes(uint8_t *bytes, size_t len);
	uint8_t readByte();
	uint8_t* readBytes(size_t len);

	uint8_t _i2cAddress;
	TwoWire *_wire;
	KeypadData *_commandData;
	void (*onCommandDataReceived)(KeypadData* cmdData);
};

#endif