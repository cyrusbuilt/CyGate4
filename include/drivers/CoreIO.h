#ifndef _COREIO_H
#define _COREIO_H

#include <Arduino.h>
#include "Adafruit_MCP23017.h"
#include "IOExpPinMap.h"
#include "LED.h"
#include "Relay.h"

// Known peripheral I2C bus addresses.
#define I2C_ADDRESS_OFFSET 32
#define RTC_ADDRESS 0x68
#define PRIMARY_EXP_ADDRESS 0

// Peripheral I/O processing core ID
#define PIO_CORE_ID 1

enum class OnboardRelaySelect : uint8_t {
	RELAY_1 = 0,
	RELAY_2 = 1,
	RELAY_3 = 2,
	RELAY_4 = 3
};

enum class OnboardOptoZoneInput : uint8_t {
	ZONE_1 = 0,
	ZONE_2 = 1,
	ZONE_3 = 2,
	ZONE_4 = 3,
	ZONE_5 = 4,
	ZONE_6 = 5,
	ZONE_7 = 6,
	ZONE_8 = 7
};

enum class OnboardDryContactInput : uint8_t {
	DOORCONTACT_1 = 0,
	DOORCONTACT_2 = 1,
	DOORCONTACT_3 = 2,
	DOORCONTACT_4 = 3,
	REX_1 = 4,
	REX_2 = 5,
	REX_3 = 6,
	REX_4 = 7
};

class CoreIOClass {
public:
	CoreIOClass();
	void init(Adafruit_MCP23017* controller);
	void armLedOn();
	void armLedOff();
	void heartbeatLedOn();
	void heartbeatLedOff();
	void heartbeatLedFlash(unsigned long delayMs);
	uint8_t readOptoZoneInput(OnboardOptoZoneInput input);
	uint8_t readDryContactZoneInput(OnboardDryContactInput input);
	void relayOn(OnboardRelaySelect relay);
	void relayOff(OnboardRelaySelect relay);

private:
	Adafruit_MCP23017* _controller;
	LED* _heartbeatLED;
	const uint8_t _localOptoInputs[6] = {
		PIN_OPTO_ZONE_1,
		PIN_OPTO_ZONE_2,
		PIN_OPTO_ZONE_3,
		PIN_OPTO_ZONE_4,
		PIN_OPTO_ZONE_5,
		PIN_OPTO_ZONE_6
	};
	const uint8_t _expOptoInputs[2] = {
		PIN_OPTO_ZONE_7,
		PIN_OPTO_ZONE_8
	};
	const uint8_t _relayOutputs[4] = {
		PIN_RELAY_1,
		PIN_RELAY_2,
		PIN_RELAY_3,
		PIN_RELAY_4
	};
	const uint8_t _dcInputs[8] = {
		PIN_DC_ZONE_1,
		PIN_DC_ZONE_2,
		PIN_DC_ZONE_3,
		PIN_DC_ZONE_4,
		PIN_REX_ZONE_1,
		PIN_REX_ZONE_2,
		PIN_REX_ZONE_3,
		PIN_REX_ZONE_4
	};
	const uint8_t _inputOnlyAnalogPins[3] = {
		PIN_EXP_A2_I,
		PIN_EXP_A3_I,
		PIN_EXP_A4_I
	};
};

extern CoreIOClass CoreIO;

#endif