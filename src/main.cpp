/**
 * 
 */

#ifndef ESP32
	#error This firmware is only compatible with ESP32 controllers.
#endif

#include <Arduino.h>
#include <FS.H>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <time.h>
#include <vector>
#include "Adafruit_MCP23017.h"
#include "ArduinoJson.h"
#include "CoreIO.h"
#include "FobReader.h"
#include "Keypad.h"
#include "PubSubClient.h"
#include "RelayModule.h"
#include "RTClib.h"
#include "TelemetryHelper.h"
#include "config.h"

#define FIRMWARE_VERSION "1.0"

using namespace std;

// Forward declarations
void onSyncClock();
void onKeypadCommand(KeypadData* cmdData);

// Global vars
#ifdef SUPPORT_MDNS
	#include <ESPmDNS.h>
	MDNSResponder mdns;
#endif
DateTime compileTime(F(__DATE__), F(__TIME__));
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Adafruit_MCP23017 primaryBus;
RTC_DS1307 rtc;
config_t config;
bool filesystemMounted = false;
bool primaryExpanderFound = false;
bool rtcFound = false;
vector<byte> devicesFound;
vector<Adafruit_MCP23017> additionalBusses;
vector<RelayModule> relayModules;
vector<FobReader> fobReaders;
vector<Keypad> keypads;
TaskHandle_t ioTask;  // TODO We really only need this if we want to delete/suspend/resume the task at some point.
volatile SystemState sysState = SystemState::BOOTING;
volatile ArmState armState = ArmState::DISARMED;

void saveConfiguration() {
	Serial.print(F("INFO: Saving configuration to "));
    Serial.print(CONFIG_FILE_PATH);
    Serial.println(F(" ... "));
    if (!filesystemMounted) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Filesystem not mount."));
        return;
    }

	StaticJsonDocument<350> doc;
    doc["hostname"] = config.hostname;
    doc["useDhcp"] = config.useDhcp;
    doc["ip"] = config.ip.toString();
    doc["gateway"] = config.gw.toString();
    doc["subnetmask"] = config.sm.toString();
    doc["dnsServer"] = config.dns.toString();
    doc["wifiSSID"] = config.ssid;
    doc["wifiPassword"] = config.password;
    doc["mqttBroker"] = config.mqttBroker;
    doc["mqttPort"] = config.mqttPort;
    doc["mqttControlChannel"] = config.mqttTopicControl;
    doc["mqttStatusChannel"] = config.mqttTopicStatus;
    doc["mqttUsername"] = config.mqttUsername;
    doc["mqttPassword"] = config.mqttPassword;
	doc["clockTimezone"] = config.clockTimezone;
	#ifdef SUPPORT_OTA
	doc["otaEnable"] = config.otaEnable;
	doc["otaPort"] = config.otaPort;
	doc["otaPassword"] = config.otaPassword;
	#endif
	#ifdef SUPPORT_MDNS
	doc["mdnsEnable"] = config.mdnsEnable;
	#endif

	File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");
    if (!configFile) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Failed to open config file for writing."));
        doc.clear();
        return;
    }

	serializeJsonPretty(doc, configFile);
    doc.clear();
    configFile.flush();
    configFile.close();
    Serial.println(F("DONE"));
}

void printWarningAndContinue(const __FlashStringHelper *message) {
    Serial.println();
    Serial.println(message);
    Serial.print(F("INFO: Continuing... "));
}

void setConfigurationDefaults() {
    String chipId = String((uint32_t)(ESP.getEfuseMac() >> 32) % 0xFFFFFFFF, HEX);
    String defHostname = String(DEFAULT_HOST_NAME) + "_" + chipId;

    config.hostname = defHostname;
    config.ip = defaultIp;
    config.mqttBroker = MQTT_BROKER;
    config.mqttPassword = "";
    config.mqttPort = MQTT_PORT;
    config.mqttTopicControl = MQTT_TOPIC_CONTROL;
    config.mqttTopicStatus = MQTT_TOPIC_STATUS;
    config.mqttUsername = "";
    config.password = DEFAULT_PASSWORD;
    config.sm = defaultSm;
    config.ssid = DEFAULT_SSID;
    config.useDhcp = false;
    config.clockTimezone = DEFAULT_TIMEZONE;
    config.dns = defaultDns;
    config.gw = defaultGw;

    #ifdef SUPPORT_OTA
		config.otaEnable = true;
        config.otaPassword = DEFAULT_OTA_PASSWORD;
        config.otaPort = DEFAULT_OTA_HOST_PORT;
    #endif

	#ifdef SUPPORT_MDNS
		config.mdnsEnable = true;
	#endif
}

void loadConfiguration() {
	memset(&config, 0, sizeof(config));

    Serial.print(F("INFO: Loading config file "));
    Serial.print(CONFIG_FILE_PATH);
    Serial.print(F(" ... "));
    if (!filesystemMounted) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Filesystem not mounted."));
        return;
    }

    if (!SPIFFS.exists(CONFIG_FILE_PATH)) {
        Serial.println(F("FAIL"));
        Serial.println(F("WARN: Config file does not exist. Creating with default config ..."));
        saveConfiguration();
        return;
    }

    File configFile = SPIFFS.open(CONFIG_FILE_PATH, "r");
    if (!configFile) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Unable to open config file. Using default config."));
        return;
    }

    size_t size = configFile.size();
    uint16_t freeMem = ESP.getMaxAllocHeap() - 512;
    if (size > freeMem) {
        Serial.println(F("FAIL"));
        Serial.print(F("ERROR: Not enough free memory to load document. Size = "));
        Serial.print(size);
        Serial.print(F(", Free = "));
        Serial.println(freeMem);
        configFile.close();
        return;
    }

    DynamicJsonDocument doc(freeMem);
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Fail to parse config file to JSON. Using default config."));
        configFile.close();
        return;
    }

    doc.shrinkToFit();
    configFile.close();

    String chipId = String((uint32_t)(ESP.getEfuseMac() >> 32) % 0xFFFFFFFF, HEX);
    String defHostname = String(DEFAULT_HOST_NAME) + "_" + chipId;

    config.hostname = doc.containsKey("hostname") ? doc["hostname"].as<String>() : defHostname;
    config.useDhcp = doc.containsKey("isDhcp") ? doc["isDhcp"].as<bool>() : false;
    
    if (doc.containsKey("ip")) {
        if (!config.ip.fromString(doc["ip"].as<String>())) {
            printWarningAndContinue(F("WARN: Invalid IP in configuration. Falling back to factory default."));
        }
    }
    else {
        config.ip = defaultIp;
    }

    if (doc.containsKey("gateway")) {
        if (!config.gw.fromString(doc["gateway"].as<String>())) {
            printWarningAndContinue(F("WARN: Invalid gateway in configuration. Falling back to factory default."));
        }
    }
    else {
        config.gw = defaultGw;
    }

    if (doc.containsKey("subnetmask")) {
        if (!config.sm.fromString(doc["subnetmask"].as<String>())) {
            printWarningAndContinue(F("WARN: Invalid subnet mask in configuration. Falling back to factory default."));
        }
    }
    else {
        config.sm = defaultSm;
    }

    if (doc.containsKey("dns")) {
        if (!config.dns.fromString(doc["dns"].as<String>())) {
            printWarningAndContinue(F("WARN: Invalid DNS IP in configuration. Falling back to factory default."));
        }
    }
    else {
        config.dns = defaultDns;
    }

    config.ssid = doc.containsKey("wifiSSID") ? doc["wifiSSID"].as<String>() : DEFAULT_SSID;
    config.password = doc.containsKey("wifiPassword") ? doc["wifiPassword"].as<String>() : DEFAULT_PASSWORD;
    config.mqttBroker = doc.containsKey("mqttBroker") ? doc["mqttBroker"].as<String>() : MQTT_BROKER;
    config.mqttPort = doc.containsKey("mqttPort") ? doc["mqttPort"].as<int>() : MQTT_PORT;
    config.mqttTopicControl = doc.containsKey("mqttControlChannel") ? doc["mqttControlChannel"].as<String>() : MQTT_TOPIC_CONTROL;
    config.mqttTopicStatus = doc.containsKey("mqttStatusChannel") ? doc["mqttStatusChannel"].as<String>() : MQTT_TOPIC_STATUS;
    config.mqttUsername = doc.containsKey("mqttUsername") ? doc["mqttUsername"].as<String>() : "";
    config.mqttPassword = doc.containsKey("mqttPassword") ? doc["mqttPassword"].as<String>() : "";
	config.clockTimezone = doc.containsKey("clockTimezone") ? doc["clockTimezone"].as<int>() : DEFAULT_TIMEZONE;

    #ifdef SUPPORT_OTA
		config.otaEnable = doc.containsKey("otaEnable") ? doc["otaEnable"].as<bool>() : true;
        config.otaPort = doc.containsKey("otaPort") ? doc["otaPort"].as<uint16_t>() : DEFAULT_OTA_HOST_PORT;
        config.otaPassword = doc.containsKey("otaPassword") ? doc["otaPassword"].as<String>() : DEFAULT_OTA_PASSWORD;
    #endif

	#ifdef SUPPORT_MDNS
		config.mdnsEnable = doc.containsKey("mdnsEnable") ? doc["mdnsEnable"].as<bool>() : true;
	#endif

    doc.clear();
    Serial.println(F("DONE"));
}

void scanBusDevices() {
    byte error;
    byte address;
    int devices = 0;

    Serial.println(F("INFO: Beginning I2C bus scan ..."));
    for (address = 0; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            devices++;
            Serial.print(F("INFO: I2C device found at address 0x"));
            if (address < 16) {
                Serial.print(F("0"));
            }

            Serial.print(address, HEX);
            Serial.println(F("!"));
            uint8_t realAddress = address - I2C_ADDRESS_OFFSET;
            if (!primaryExpanderFound && realAddress == PRIMARY_EXP_ADDRESS) {
                primaryExpanderFound = true;
            }
			else if (!rtcFound && address == RTC_ADDRESS) {
				rtcFound = true;
			}
            else {
                devicesFound.push_back(realAddress);
            }
        }
        else if (error == 4) {
            Serial.print(F("ERROR: Unknown error at address 0x"));
            if (address < 16) {
                Serial.print(F("0"));
            }

            Serial.println(address, HEX);
        }
    }

    if (devices == 0) {
        Serial.println(F("ERROR: No devices found!"));
    }
    else {
        Serial.println(F("INFO: Scan complete."));
    }
}

void onKeypadCommand(KeypadData* cmdData) {
    String key;
    uint8_t readCard[cmdData->size];
    for (uint8_t i = 0; i < cmdData->size; i++) {
        key += String((char)cmdData->data[i], HEX);
    }

    switch (cmdData->command) {
        case (uint8_t)KeypadCommands::ARM_AWAY:

            break;
        case (uint8_t)KeypadCommands::ARM_STAY:

            break;
        case (uint8_t)KeypadCommands::DISARM:

            break;
        case (uint8_t)KeypadCommands::LOCK:

            break;
        case (uint8_t)KeypadCommands::UNLOCK:

            break;
        default:
            break;
    }
}

void initSerial() {
	#ifdef DEBUG
	const bool debug = true;
	#else
	const bool debug = false;
	#endif
    Serial.setDebugOutput(debug);
    Serial.begin(SERIAL_BAUD, SERIAL_8N1);
    Serial.println();
    Serial.println();
    Serial.print(F("INIT: CyGate4 v"));
    Serial.print(FIRMWARE_VERSION);
    Serial.println(F(" booting ..."));
    Serial.println();
}

void initSys() {
	Serial.print(F("INIT: Firmware compiled: "));
	Serial.println(compileTime.timestamp(DateTime::TIMESTAMP_FULL));
	Serial.print(F("INIT: MCU Revision: "));
	Serial.println(ESP.getChipRevision());
	Serial.print(F("INIT: CPU Frequency: "));
	Serial.print(ESP.getCpuFreqMHz());
	Serial.println(F(" MHz"));
	Serial.println(F("INIT: CPU Cores: 2"));
	Serial.print(F("INIT: Chip revision: "));
	Serial.println(ESP.getChipRevision());
	Serial.print(F("INIT: Mem: "));
	Serial.println(ESP.getFlashChipSize());
	Serial.print(F("INIT: SDK Version: "));
	Serial.println(ESP.getSdkVersion());
	delay(1000);
}

void initComBus() {
	Serial.println(F("INIT: Initializing communication busses..."));
	Wire.begin();
	scanBusDevices();
	if (primaryExpanderFound) {
		Serial.println(F("INFO: Found primary host bus controller."));
        Serial.print(F("INFO: Secondary bus controllers found: "));
        Serial.println(devicesFound.size());

		uint8_t addr = 0;
		additionalBusses.push_back(primaryBus);  // The primary bus controller should always be at index 0.
		for (std::size_t i = 0; i < devicesFound.size(); i++) {
			// MCP23017 addresses are 32 - 40, with 32 reserved by CoreIO.
			addr = devicesFound.at(i);
			if (addr >= 33 && addr <= 40) {
            	Adafruit_MCP23017 newBus;
            	newBus.begin(addr);
            	additionalBusses.push_back(newBus);
			}
        }
	}
	else {
		Serial.println(F("ERROR: Primary host bus controller not found!!"));
		// TODO HCF?
	}

	Serial.println(F("INIT: Comm bus initialization complete."));
}

void initCoreIO() {
	Serial.print(F("INIT: Initializing CoreIO at address "));
	Serial.print(PRIMARY_EXP_ADDRESS, HEX);
	Serial.println(F(" ..."));
	CoreIO.init(&primaryBus);
	CoreIO.heartbeatLedOn();
	CoreIO.armLedOn();
	Serial.println(F("DONE"));
	delay(200);
}

void initRTC() {
	Serial.print(F("INIT: Initializing RTC at address 0x"));
	Serial.print(RTC_ADDRESS, HEX);
	Serial.println(F(" ..."));
	if (!rtcFound || !rtc.begin()) {
		Serial.println(F("FAIL"));
		Serial.println(F("ERROR: Hardware RTC not found."));
		return;
	}

	Serial.println(F("DONE"));
	if (!rtc.isrunning()) {
		Serial.println(F("INIT: RTC is not running. Defaulting to compile time."));
		rtc.adjust(compileTime);
	}
}

void initCardReaders() {
	Serial.print(F("INIT: Initializing fob readers... "));
	if (devicesFound.size() == 0) {
		Serial.println(F("NONE FOUND"));
		return;
	}
	
	uint8_t count = 0;
	uint8_t addr = 0;
	for (std::size_t i = 0; i < devicesFound.size(); i++) {
		if (i >= 120 && i <= 127) {
			addr = devicesFound.at(i);

			FobReader reader;
			reader.begin(addr);
			if (reader.detect()) {
				if (count == 0) {
					Serial.println();
				}

				Serial.print(F("INIT: Initializing fob reader at address 0x"));
				Serial.println(reader.getAddress(), HEX);
				reader.init();  // TODO init returns a status code
				fobReaders.push_back(reader);
			}
		}
	}

	if (count > 0) {
		Serial.print(F("INIT: Finished initializing "));
		Serial.print(count);
		Serial.println(F(" readers."));
	}
	else {
		Serial.println(F("NONE FOUND"));
	}
}

void initRelayModules() {
	Serial.print(F("INIT: Initializing relay modules... "));
	if (additionalBusses.size() == 0) {
		Serial.println(F("NONE FOUND"));
		return;
	}

	uint8_t count = 0;
	for (std::size_t i = 0; i < additionalBusses.size(); i++) {
		RelayModule rm(&additionalBusses.at(i));
		if (rm.detect()) {
			if (count == 0) {
				Serial.println();
			}

			Serial.print(F("INIT: Initializing relay module at additional bus index "));
			Serial.println(i);
			rm.init();
			relayModules.push_back(rm);
		}
	}

	if (count > 0) {
		Serial.print(F("INIT: Finished initializing "));
		Serial.print(count);
		Serial.println(F(" relay modules."));
	}
	else {
		Serial.println(F("NONE FOUND"));
	}
}

void initKeypads() {
    Serial.print(F("INIT: Initializing keypads ..."));
    if (devicesFound.size() == 0) {
		Serial.println(F("NONE FOUND"));
		return;
	}
	
	uint8_t count = 0;
	uint8_t addr = 0;
	for (std::size_t i = 0; i < devicesFound.size(); i++) {
        if (i >= 80 && i < 88) {
            addr = devicesFound.at(i);

            Keypad keypad;
            keypad.begin(addr);
            if (keypad.detect()) {
                if (count == 0) {
                    Serial.println();
                }

                Serial.print(F("INIT: Initializing keypad at address 0x"));
                Serial.println(keypad.getAddress(), HEX);
                keypad.setCommandHandler(onKeypadCommand);
                keypad.init();  // TODO init() returns status value.
                keypads.push_back(keypad);
            }
        }
    }

    if (count == 0) {
        Serial.print(F("INIT: Finished initializing "));
        Serial.print(count);
        Serial.println(F(" keypads."));
    }
    else {
        Serial.println(F("NONE FOUND"));
    }
}

void initFilesystem() {
    Serial.print(F("INIT: Initializing SPIFFS and mounting filesystem... "));
    if (!SPIFFS.begin()) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Unable to mount filesystem."));
        return;
    }

    filesystemMounted = true;
    Serial.println(F("DONE"));
    setConfigurationDefaults();
    loadConfiguration();
}

void peripheralIoLoop(void *pvParameters) {
    // NOTE: This loop is for peripheral I/O and runs on Core 0.
}

void initTaskCores() {
    Serial.println(F("INIT: Initializing task cores..."));
    Serial.print(F("INIT: Supervisor running on core "));
    Serial.println(xPortGetCoreID());

    xTaskCreatePinnedToCore(
        peripheralIoLoop,
        "peripheralIoLoop",
        10000,
        NULL,
        1,
        &ioTask,
        PIO_CORE_ID
    );

    Serial.print(F("INIT: Peripheral I/O running on core "));
    Serial.println(PIO_CORE_ID);
}

void setup() {
	initSerial();
	initSys();
	initComBus();
	initCoreIO();
	initRTC();
	initCardReaders();
	initRelayModules();
    initKeypads();
	initFilesystem();

    initTaskCores();
}

void loop() {
    // NOTE: On ESP32, loop() runs on Core 1.
}