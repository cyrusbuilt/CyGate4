#ifndef _RELAY_MODULE_H
#define _RELAY_MODULE_H

#include <Arduino.h>
#include "Adafruit_MCP23017.h"
#include "IOExpPinMap.h"

enum class RelaySelect : uint8_t {
	RELAY1 = 1,
	RELAY2 = 2,
	RELAY3 = 3,
	RELAY4 = 4,
	RELAY5 = 5
};

enum class ModuleRelayState : uint8_t {
	OPEN = LOW,
	CLOSED = HIGH
};

class RelayModule
{
public:
	RelayModule(Adafruit_MCP23017 *busController);
	bool detect();
	void init();
	ModuleRelayState getState(RelaySelect relay);
	void setState(RelaySelect relay, ModuleRelayState state);
	bool isOpen(RelaySelect relay);
	bool isClosed(RelaySelect relay);
	void close(RelaySelect relay);
	void open(RelaySelect relay);
	void allRelaysClose();
	void allRelaysOpen();
private:
	uint8_t getRelayAddress(RelaySelect relay);
	uint8_t getLedAddressForRelay(RelaySelect relay);
	Adafruit_MCP23017 *_busController;
};

#endif