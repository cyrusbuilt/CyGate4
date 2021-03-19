#include "Doors.h"

DoorManagerClass::DoorManagerClass() {}

void DoorManagerClass::attachDoor(Door door) {
	if (_doors.size() < 4) {
		_doors.push_back(door);
	}
}

void DoorManagerClass::attachReader(uint8_t doorId, Reader reader) {
	if (doorId >= 0 && doorId < _doors.size()) {
		_doors.at(doorId).readers.push_back(reader);
	}
}

void DoorManagerClass::attachKeypad(uint8_t doorId, DoorKeypad keypad) {
	if (doorId >= 0 && doorId < _doors.size()) {
		_doors.at(doorId).keypads.push_back(keypad);
	}
}

void DoorManagerClass::attachLockRelay(uint8_t doorId, LockRelay relay) {
	if (doorId >= 0 && doorId < _doors.size()) {
		_doors.at(doorId).lockRelay = relay;
	}
}

DoorState DoorManagerClass::getDoorState(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		return _doors.at(doorId).state;
	}

	return DoorState::UNKNOWN;
}

LockState DoorManagerClass::getLockState(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		return _doors.at(doorId).lockState;
	}

	return LockState::UNKNOWN;
}

void DoorManagerClass::enableDoor(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		_doors.at(doorId).enabled = true;
		// TODO Fire handler indicating status change?
	}
}

void DoorManagerClass::disableDoor(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		_doors.at(doorId).enabled = false;
		// TODO Fire handler indicating status change?
	}
}

void DoorManagerClass::lockDoor(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		Door d = _doors.at(doorId);
		if (d.enabled) {
			d.lockState = LockState::LOCKED;
			// TODO Fire handler for triggering relay?
		}
	}
}

void DoorManagerClass::unlockDoor(uint8_t doorId) {
	if (doorId >= 0 && doorId < _doors.size()) {
		Door d = _doors.at(doorId);
		if (d.enabled) {
			d.lockState = LockState::UNLOCKED;
			// TODO Fire handler for triggering relay?
		}
	}
}

void DoorManagerClass::clearDoors() {
	for (auto door = _doors.begin(); door != _doors.end(); door++) {
		door->inputs.clear();
		door->keypads.clear();
		door->readers.clear();
	}

	_doors.clear();
}

vector<Door> DoorManagerClass::getDoors() {
	return _doors;
}

DoorManagerClass DoorManager;