#ifndef _APP_H
#define _APP_H

#include <Arduino.h>
#include <time.h>
#include <vector>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include "Adafruit_MCP23017.h"
#include "config.h"
#include "LED.h"
#include "NTPClient.h"
#include "PubSubClient.h"
#include "RTClib.h"
#include "TelemetryHelper.h"

#include "drivers/CoreIO.h"
#include "drivers/FobReader.h"
#include "drivers/Keypad.h"
#include "drivers/RelayModule.h"

#include "tasks/TaskApplication.h"
#include "tasks/TaskCheckKeypads.h"
#include "tasks/TaskCheckFobReaders.h"
#include "tasks/TaskSyncClock.h"
#include "tasks/TaskCheckWiFi.h"
#include "tasks/TaskCheckMqtt.h"
#include "tasks/TaskHeartBeat.h"
#include "tasks/TaskInput.h"

#ifdef SUPPORT_MDNS
#include <ESPmDNS.h>
#endif

#define FIRMWARE_VERSION "1.0"

using namespace std;

class Application
{
// *** Public members ***
public:
	static Application *singleton;
	TaskHandle_t applicationTask;
	TaskHandle_t heartbeatTask;
	TaskHandle_t keypadCheckTask;
	TaskHandle_t fobReaderCheckTask;
	TaskHandle_t clockSyncTask;
	TaskHandle_t wifiCheckTask;
	TaskHandle_t mqttCheckTask;
	TaskHandle_t inputTask;
	QueueHandle_t keypadQueue;
	QueueHandle_t fobReaderQueue;
	QueueHandle_t dryContactQueue;
	QueueHandle_t optoContactQueue;
	vector<Keypad> keypads;
	vector<FobReader> fobReaders;
	NTPClient *timeClient;
	RTC_DS1307 rtc;
	volatile SystemState sysState = SystemState::BOOTING;

public:
	Application();
	void initKeypads();
	void initFobReaders();
	void initRTC();
	void initTimeclient();
	void initWiFi();
	void initMQTT();
	void init();
	void update();
	void reboot();
	void onKeypadCommand(KeypadData* cmdData);
	void onCheckMqtt();
	static void keypadCommandCallback(KeypadData* cmdData, void* thisPointer);
	void failSafe();
	void getAvailableNetworks();
	void doFactoryRestore();
	void handleNewHostname(const char* newHostname);
	void onMqttMessage(char* topic, byte* payload, unsigned int length);
	void handleSwitchToDhcp();
	void handleSwitchToStatic(IPAddress newIp, IPAddress newSm, IPAddress newGw, IPAddress newDns);
	void onCheckWiFi();
	void handleReconnectFromConsole();
	void handleWifiConfig(String newSsid, String newPassword);
	void handleSaveConfig();
	void handleMqttConfigCommand(String newBroker, int newPort, String newUsername, String newPassw, String newConChan, String newStatChan);
	void handleBusResetCommand();

private:
	WiFiClient wifiClient;
	PubSubClient mqttClient;
	Adafruit_MCP23017 primaryBus;
	WiFiUDP ntpUdp;
	config_t config;
	bool filesystemMounted = false;
	bool primaryExpanderFound = false;
	bool rtcFound = false;
	vector<byte> devicesFound;
	vector<Adafruit_MCP23017> additionalBusses;
	vector<RelayModule> relayModules;
	volatile ArmState armState = ArmState::DISARMED;

private:
	void printNetworkInfo();
	void publishSystemState();
	void saveConfiguration();
	void printWarningAndContinue(const __FlashStringHelper *message);
	void setConfigurationDefaults();
	void loadConfiguration();
	void scanBusDevices();
	void resetCommBus();
	void connectWifi();
	void handleControlRequest(ControlCommand command);
	bool reconnectMqttClient();
	void initSys();
	void initCommBus();
	void initCoreIO();
	void initCardReaders();
	void initRelayModules();
	void initFilesystem();
	void initMDNS();
	void initOTA();
	void initConsole();
};

#endif