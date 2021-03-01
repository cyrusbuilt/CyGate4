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
	REQUEST_STATUS = 3
};

class TelemetryHelper
{
public:
    static String getMqttStateDesc(int state);
};

#endif