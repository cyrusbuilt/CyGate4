#ifndef _DOORS_H
#define _DOORS_H

#include <Arduino.h>
#include <vector>

using namespace std;

enum class LockState : uint8_t {
	UNLOCKED = 0,
	LOCKED = 1,
	UNKNOWN = 2
};

enum class DoorState : uint8_t {
	OPEN = 0,
	CLOSED = 1,
	UNKNOWN = 2
};

enum class InputType : uint8_t {
	DOORCONTACT = 0,
	REX = 1,
	BUTTON = 2,
	CRASHBAR = 3,
	MOTION = 4,
	GLASSBREAK = 5,
	SMOKE = 6
};

struct DoorInput {
	InputType type;
	uint8_t moduleId;
	uint8_t inputId;
};

struct Reader {
	uint8_t id;
};

struct DoorKeypad {
	uint8_t id;
};

struct LockRelay {
	uint8_t relayId;
	uint8_t moduleId;
};

struct Door {
	const char* name;
	vector<Reader> readers;
	vector<DoorKeypad> keypads;
	vector<DoorInput> inputs;
	LockRelay lockRelay;
	bool enabled;
	DoorState state;
	LockState lockState;
};

class DoorManagerClass {
public:
	DoorManagerClass();
	void attachDoor(Door door);
	void attachReader(uint8_t doorId, Reader reader);
	void attachKeypad(uint8_t doorId, DoorKeypad keypad);
	void attachLockRelay(uint8_t doorId, LockRelay relay);
	DoorState getDoorState(uint8_t doorId);
	LockState getLockState(uint8_t doorId);
	void enableDoor(uint8_t doorId);
	void disableDoor(uint8_t doorId);
	void lockDoor(uint8_t doorId);
	void unlockDoor(uint8_t doorId);
	void clearDoors();
	vector<Door> getDoors();

private:
	vector<Door> _doors;
};

extern DoorManagerClass DoorManager;

#endif