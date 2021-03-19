#ifndef TelemetryHelper_h
#define TelemetryHelper_h

#include <Arduino.h>

enum class SystemState : uint8_t {
	BOOTING = 0,
	NORMAL = 1,
	UPDATING = 2,
	SYS_DISABLED = 3
};

enum class ArmState : uint8_t {
	DISARMED = 0,
	ARMED_STAY = 1,
	ARMED_AWAY = 2
};

enum class ControlCommand : uint8_t {
	DISABLE = 0,
	ENABLE = 1,
	REBOOT = 2,
	REQUEST_STATUS = 3,
	SET_ARM_STATE = 4,
	LOCK_DOOR = 5,
	UNLOCK_DOOR = 6
};

// TODO relay mapping
// Module ID (0 is always onboard) -> relay address -> purpose
// Read definitions from inputs.json. Any inputs not in file are "undefined".

// TODO input mapping
// Module ID (0 is always onbard) -> address -> type -> purpose
// Read definitions from outputs.json. Any outputs not in file are "undefined".

// TODO action mapping
// Event (keypad command, fob read, input trigger, schedule trigger) -> 

class TelemetryHelper
{
public:
    static String getMqttStateDesc(int state);
};

#endif