#include "App.h"

#include <FS.H>
#include <SPIFFS.h>
#include <Wire.h>

#include "ArduinoJson.h"
#include "Console.h"
#include "Doors.h"
#include "ESPCrashMonitor-master/ESPCrashMonitor.h"
#include "ResetManager.h"

#define PIN_BUS_RESET 16

IPAddress defaultIp(192, 168, 0, 235);
IPAddress defaultGw(192, 168, 0, 1);
IPAddress defaultSm(255, 255, 255, 0);
IPAddress defaultDns(defaultGw);
DateTime compileTime(F(__DATE__), F(__TIME__));

void doReboot() {
    Application::singleton->reboot();
}

void appGetAvailableNetworks() {
    Application::singleton->getAvailableNetworks();
}

void appDoFactoryRestore() {
    Application::singleton->doFactoryRestore();
}

void appHandleNewHostname(const char* newHostname) {
    Application::singleton->handleNewHostname(newHostname);
}

void appOnMqttMessage(char* topic, byte* payload, unsigned int length) {
    Application::singleton->onMqttMessage(topic, payload, length);
}

void appHandleSwitchToDhcp() {
    Application::singleton->handleSwitchToDhcp();
}

void appHandleSwitchToStatic(IPAddress newIp, IPAddress newSm, IPAddress newGw, IPAddress newDns) {
    Application::singleton->handleSwitchToStatic(newIp, newSm, newGw, newDns);
}

void appHandleReconnectFromConsole() {
    Application::singleton->handleReconnectFromConsole();
}

void appHandleWifiConfig(String newSsid, String newPassword) {
    Application::singleton->handleWifiConfig(newSsid, newPassword);
}

void appHandleSaveConfig() {
    Application::singleton->handleSaveConfig();
}

void appHandleMqttConfigCommand(String newBroker, int newPort, String newUsername, String newPassw, String newConChan, String newStatChan) {
    Application::singleton->handleMqttConfigCommand(newBroker, newPort, newUsername, newPassw, newConChan, newStatChan);
}

void appHandleBusResetCommand() {
    Application::singleton->handleBusResetCommand();
}

Application* Application::singleton = nullptr;

Application::Application() {
	// TODO don't forget to call setClient() on mqttClient!
	this->singleton = this;
}

void Application::printNetworkInfo() {
    Serial.print(F("INFO: Local IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("INFO: Gateway: "));
    Serial.println(WiFi.gatewayIP());
    Serial.print(F("INFO: Subnet mask: "));
    Serial.println(WiFi.subnetMask());
    Serial.print(F("INFO: DNS server: "));
    Serial.println(WiFi.dnsIP());
    Serial.print(F("INFO: MAC address: "));
    Serial.println(WiFi.macAddress());
    #ifdef DEBUG
    WiFi.printDiag(Serial);
    #endif
}

void Application::getAvailableNetworks() {
    //ESPCrashMonitor.defer();
    Serial.println(F("INFO: Scanning WiFi networks..."));
    int numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks; i++) {
        Serial.print(F("ID: "));
        Serial.print(i);
        Serial.print(F("\tNetwork name: "));
        Serial.print(WiFi.SSID(i));
        Serial.print(F("\tSignal strength:"));
        Serial.println(WiFi.RSSI(i));
    }
    Serial.println(F("----------------------------------"));
}

void Application::reboot() {
    Serial.println(F("INFO: Rebooting..."));
    Serial.flush();
    delay(1000);
    ResetManager.softReset();
}

void Application::publishSystemState() {
    if (mqttClient.connected()) {
        CoreIO.heartbeatLedOn();
        uint16_t freeMem = ESP.getFreeHeap() - 512;

        DynamicJsonDocument doc(freeMem);
        doc["client_id"] = config.hostname;
        doc["systemState"] = (uint8_t)sysState;
        doc["firmwareVersion"] = FIRMWARE_VERSION;
        doc["armState"] = (uint8_t)armState;

        JsonArray theDoors = doc.createNestedArray("doors");
        auto doors = DoorManager.getDoors();
        for (auto d = doors.begin(); d != doors.end(); d++) {
            JsonObject obj;
            obj["name"] = d->name;
            obj["state"] = (uint8_t)d->state;
            obj["enabled"] = d->enabled ? 1 : 0;
            obj["lockState"] = (uint8_t)d->lockState;

            if (&d->lockRelay != nullptr) {
                JsonObject rel = obj.createNestedObject("lockRelay");
                rel["moduleId"] = d->lockRelay.moduleId;
                rel["relayId"] = d->lockRelay.relayId;
            }

            JsonArray readers = obj.createNestedArray("readers");
            if (d->readers.size() > 0) {
                for (auto r = d->readers.begin(); r != d->readers.end(); r++) {
                    JsonObject rdr;
                    rdr["id"] = r->id;
                    readers.add(rdr);
                }
            }

            JsonArray keypads = obj.createNestedArray("keypads");
            if (d->keypads.size() > 0) {
                for (auto k = d->keypads.begin(); k != d->keypads.end(); k++) {
                    JsonObject kpd;
                    kpd["id"] = k->id;
                    keypads.add(kpd);
                }
            }

            JsonArray inputs = obj.createNestedArray("inputs");
            if (d->inputs.size() > 0) {
                for (auto i = d->inputs.begin(); i != d->inputs.end(); i++) {
                    JsonObject inp;
                    inp["type"] = (uint8_t)i->type;
                    inp["moduleId"] = i->moduleId;
                    inp["inputId"] = i->inputId;
                    inputs.add(inp);
                }
            }

            theDoors.add(obj);
        }
        
        doc.shrinkToFit();

        String jsonStr;
        size_t len = serializeJson(doc, jsonStr);
        Serial.print(F("INFO: Publishing system state: "));
        Serial.println(jsonStr);
        if (!mqttClient.publish(config.mqttTopicStatus.c_str(), jsonStr.c_str(), len)) {
            Serial.println(F("ERROR: Failed to publish message."));
        }

        doc.clear();
        CoreIO.heartbeatLedOff();
    }
}

void Application::saveConfiguration() {
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

void Application::printWarningAndContinue(const __FlashStringHelper *message) {
    Serial.println();
    Serial.println(message);
    Serial.print(F("INFO: Continuing... "));
}

void Application::setConfigurationDefaults() {
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

void Application::loadConfiguration() {
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

void Application::loadDoors() {
    Serial.print(F("INFO: Loading door file "));
    Serial.print(DOOR_FILE_PATH);
    Serial.print(F(" ... "));
    if (!filesystemMounted) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Filesystem not mounted."));
        return;
    }

    if (!SPIFFS.exists(DOOR_FILE_PATH)) {
        Serial.println(F("FAIL"));
        Serial.println(F("WARN: No door file found. Skipping..."));
        return;
    }

    File doorFile = SPIFFS.open(DOOR_FILE_PATH, "r");
    if (!doorFile) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Unable to open door file."));
        return;
    }

    size_t size = doorFile.size();
    uint16_t freeMem = ESP.getMaxAllocHeap() - 512;
    if (size > freeMem) {
        Serial.println(F("FAIL"));
        Serial.print(F("ERROR: Not enough free memory to load door file. Size = "));
        Serial.print(size);
        Serial.print(F(", Free = "));
        Serial.println(freeMem);
        doorFile.close();
        return;
    }

    DynamicJsonDocument doc(freeMem);
    DeserializationError error = deserializeJson(doc, doorFile);
    if (error) {
        Serial.println(F("FAIL"));
        Serial.println(F("ERROR: Fail to parse door file to JSON."));
        doorFile.close();
        return;
    }

    doc.shrinkToFit();
    doorFile.close();

    if (doc.containsKey("doors")) {
        DoorManager.clearDoors();
        JsonArray doors = doc["doors"];
        for (auto d : doors) {
            Door theDoor;
            theDoor.name = d["name"].as<const char*>();
            // TODO load the remaining model
        }
    }

    doc.clear();
}

void Application::doFactoryRestore() {
    Serial.println();
    Serial.println(F("Are you sure you wish to restore to factory default? (Y/n)"));
    Console.waitForUserInput();
    
    String str = Console.getInputString();
    if (str == "Y" || str == "y") {
        Serial.print(F("INFO: Clearing current config... "));
        if (filesystemMounted) {
            if (SPIFFS.remove(CONFIG_FILE_PATH)) {
                Serial.println(F("DONE"));
                Serial.print(F("INFO: Removed file: "));
                Serial.println(CONFIG_FILE_PATH);

                Serial.print(F("INFO: Rebooting in "));
                for (uint8_t i = 5; i >= 1; i--) {
                    Serial.print(i);
                    Serial.print(F(" "));
                    delay(1000);
                }

                reboot();
            }
            else {
                Serial.println(F("FAIL"));
                Serial.println(F("ERROR: Failed to delete cofiguration file."));
            }
        }
        else {
            Serial.println(F("FAIL"));
            Serial.println(F("ERROR: Filesystem not mounted."));
        }
    }

    Serial.println();
}

void Application::failSafe() {
    ESPCrashMonitor.defer();
    Serial.println();
    Serial.println(F("ERROR: Entering failsafe (config) mode..."));
    CoreIO.heartbeatLedOn();
    sysState = SystemState::SYS_DISABLED;
    publishSystemState();
    Console.enterCommandInterpreter();
}

void Application::connectWifi() {
    if (config.hostname) {
        WiFi.setHostname(config.hostname.c_str());
    }

    #ifdef DEBUG
    Serial.println(F("DEBUG: Setting mode..."));
    #endif
    WiFi.mode(WIFI_STA);

    #ifdef DEBUG
    Serial.println(F("DEBUG: Disconnect and clear to prevent auto connect..."));
    #endif
    WiFi.persistent(false);
    WiFi.disconnect(true);
    ESPCrashMonitor.defer();

    delay(1000);
    if (config.useDhcp) {
        // If set to all zeros, then the SDK assumes DHCP.
        WiFi.config(0U, 0U, 0U, 0U);
    }
    else {
        // If actual IP set, then disables DHCP and assumes static.
        WiFi.config(config.ip, config.gw, config.sm, config.dns);
    }

    #ifdef DEBUG
    Serial.println(F("DEBUG: Beginning connection..."));
    #endif
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    #ifdef DEBUG
    Serial.println(F("DEBUG: Waiting for connection..."));
    #endif
    
    const int maxTries = 20;
    int currentTry = 0;
    while ((WiFi.status() != WL_CONNECTED) && (currentTry < maxTries)) {
        ESPCrashMonitor.iAmAlive();
        currentTry++;
        CoreIO.heartbeatLedFlash(500);
        delay(500);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("ERROR: Failed to connect to WiFi!"));
        failSafe();
    }
    else {
        printNetworkInfo();
    }
}

void Application::scanBusDevices() {
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

void Application::resetCommBus() {
    Serial.print(F("INFO: Resetting comm bus... "));
    digitalWrite(PIN_BUS_RESET, LOW);
    delay(500);
    digitalWrite(PIN_BUS_RESET, HIGH);
    Serial.println(F("DONE"));
}

void Application::onKeypadCommand(KeypadData* cmdData) {
    String key;
    for (uint8_t i = 0; i < cmdData->size; i++) {
        key += String((char)cmdData->data[i], HEX);
    }

    // TODO validate key. If invalid, send key rejection
    // back and take no other action.

    switch ((KeypadCommands)cmdData->command) {
        case KeypadCommands::ARM_AWAY:
            armState = ArmState::ARMED_AWAY;
            CoreIO.armLedOn();
            // TODO what else to do?
            break;
        case KeypadCommands::ARM_STAY:
            armState = ArmState::ARMED_STAY;
            CoreIO.armLedOn();
            // TODO what else to do?
            break;
        case KeypadCommands::DISARM:
            armState = ArmState::DISARMED;
            CoreIO.armLedOff();
            // TODO what else to do?
            break;
        case KeypadCommands::LOCK:

            break;
        case KeypadCommands::UNLOCK:

            break;
        default:
            break;
    }
}

void Application::onFobRead(Tag* tagData) {
    for (uint8_t i = 0; i < tagData->records; i++) {
        String key;
        for (uint8_t j = 0; j < tagData->size; j++) {
            key += String((char)tagData->tagBytes[i], HEX);
        }

        // TODO Check key validity then fire appropriate action.
    }
}

void Application::handleControlRequest(ControlCommand command) {
    // TODO handle incoming commands.
}

void Application::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    CoreIO.heartbeatLedOn();
    Serial.print(F("INFO: [MQTT] Message arrived: ["));
    Serial.print(topic);
    Serial.print(F("] "));

    // It's a lot easier to deal with if we just convert the payload
    // to a string first.
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    Serial.println(msg);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        Serial.print(F("ERROR: Failed to parse MQTT message to JSON: "));
        Serial.println(error.c_str());
        doc.clear();
        return;
    }

    if (doc.containsKey("client_id")) {
        String id = doc["client_id"].as<String>();
        id.toUpperCase();
        if (!id.equals(config.hostname)) {
            Serial.println(F("INFO: Control message not intended for this host. Ignoring..."));
            doc.clear();
            return;
        }
    }
    else {
        Serial.println(F("WARN: MQTT message does not contain client ID. Ignoring..."));
        doc.clear();
        return;
    }

    if (!doc.containsKey("command")) {
        Serial.println(F("WARN: MQTT message does not contain a control command. Ignoring..."));
        doc.clear();
        return;
    }

    ControlCommand cmd = (ControlCommand)doc["command"].as<uint8_t>();
    handleControlRequest(cmd);
}

bool Application::reconnectMqttClient() {
    if (!mqttClient.connected()) {
        CoreIO.heartbeatLedOn();
        Serial.print(F("INFO: Attempting to establish MQTT connection to "));
        Serial.print(config.mqttBroker);
        Serial.print(F(" on port: "));
        Serial.print(config.mqttPort);
        Serial.println(F("..."));
        
        bool didConnect = false;
        if (config.mqttUsername.length() > 0 && config.mqttPassword.length() > 0) {
            didConnect = mqttClient.connect(config.hostname.c_str(), config.mqttUsername.c_str(), config.mqttPassword.c_str());
        }
        else {
            didConnect = mqttClient.connect(config.hostname.c_str());
        }

        if (didConnect) {
            Serial.print(F("INFO: Subscribing to channel: "));
            Serial.println(config.mqttTopicControl);
            mqttClient.subscribe(config.mqttTopicControl.c_str());

            Serial.print(F("INFO: Publishing to channel: "));
            Serial.println(config.mqttTopicStatus);
        }
        else {
            String failReason = TelemetryHelper::getMqttStateDesc(mqttClient.state());
            Serial.print(F("ERROR: Failed to connect to MQTT broker: "));
            Serial.println(failReason);
            return false;
        }

        CoreIO.heartbeatLedOff();
    }

    return true;
}

void Application::onCheckMqtt() {
    Serial.println(F("INFO: Checking MQTT connection status..."));
    if (reconnectMqttClient()) {
        Serial.println(F("INFO: Successfully reconnected to MQTT broker."));
        publishSystemState();
    }
    else {
        Serial.println(F("ERROR: MQTT connection lost and reconnect failed."));
        Serial.print(F("INFO: Retrying connection in "));
        Serial.print(CHECK_MQTT_INTERVAL % 1000);
        Serial.println(F(" seconds."));
    }
}

void Application::handleSwitchToDhcp() {
    if (config.useDhcp) {
        Serial.println(F("INFO: DHCP mode already set. Skipping..."));
        Serial.println();
    }
    else {
        config.useDhcp = true;
        Serial.println(F("INFO: Set DHCP mode."));
        WiFi.config(0U, 0U, 0U, 0U);
    }
}

void Application::initSys() {
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

void Application::initCommBus() {
	Serial.println(F("INIT: Initializing communication busses..."));

    // ******* CRITICAL *******
    // We MUST drive the reset pin on the MCP23017 high or it will
    // not function. But, tying the reset pin to an output allows
    // us to programattically reset the I/O expander if needed.
    pinMode(PIN_BUS_RESET, OUTPUT);
    digitalWrite(PIN_BUS_RESET, HIGH);
    // ************************

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

void Application::initCoreIO() {
	Serial.print(F("INIT: Initializing CoreIO at address "));
	Serial.print(PRIMARY_EXP_ADDRESS, HEX);
	Serial.println(F(" ..."));
	CoreIO.init(&primaryBus);
	CoreIO.heartbeatLedOn();
	CoreIO.armLedOn();
	Serial.println(F("DONE"));
	delay(200);
}

void Application::initRTC() {
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

void Application::initCardReaders() {
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

void Application::initRelayModules() {
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

void Application::initKeypads() {
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

void Application::initFobReaders() {
    Serial.print(F("INIT: Initializing fob readers ..."));
    if (devicesFound.size() == 0) {
		Serial.println(F("NONE FOUND"));
		return;
	}
	
	uint8_t count = 0;
	uint8_t addr = 0;
	for (std::size_t i = 0; i < devicesFound.size(); i++) {
        if (i >= 80 && i < 88) {
            addr = devicesFound.at(i);

            FobReader reader;
            reader.begin(addr);
            if (reader.detect()) {
                if (count == 0) {
                    Serial.println();
                }

                Serial.print(F("INIT: Initializing fob reader at address 0x"));
                Serial.println(reader.getAddress(), HEX);
                reader.init();  // TODO init() returns status value.
                Serial.print(F("INIT: fob reader firmware version: "));
                Serial.println(reader.getFirmwareVersion());
                Serial.print(F("INIT: Fob reader MF version: 0x"));
                Serial.println(reader.getMiFareVersion(), HEX);
                Serial.print(F("INIT: Performing reader self-test... "));
                if (reader.selfTest() == 0x01) {  // TODO is this right?
                    Serial.println("PASS");
                    fobReaders.push_back(reader);
                }
                else {
                    Serial.println(F("FAIL"));
                    Serial.println(F("ERROR: Ignoring fob reader at address 0x"));
                    Serial.println(reader.getAddress(), HEX);
                }
            }
        }
    }

    if (count == 0) {
        Serial.print(F("INIT: Finished initializing "));
        Serial.print(count);
        Serial.println(F(" fob readers."));
    }
    else {
        Serial.println(F("NONE FOUND"));
    }
}

void Application::initFilesystem() {
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

void Application::initWiFi() {
    Serial.println(F("INIT: Initializing WiFi... "));
    getAvailableNetworks();
    
    Serial.print(F("INFO: Connecting to SSID: "));
    Serial.print(config.ssid);
    Serial.println(F("..."));
    
    connectWifi();
}

void Application::initMDNS() {
    #ifdef SUPPORT_MDNS
        Serial.print(F("INIT: Starting MDNS responder... "));
        if (WiFi.status() == WL_CONNECTED) {
            ESPCrashMonitor.defer();
            delay(500);
            if (!MDNS.begin(config.hostname.c_str())) {
                Serial.println(F(" FAILED"));
                return;
            }
            
            #ifdef SUPPORT_OTA
                MDNS.addService(config.hostname, "ota", config.otaPort);
            #endif
            Serial.println(F(" DONE"));
        }
        else {
            Serial.println(F(" FAILED"));
        }
    #endif
}

void Application::initOTA() {
    #ifdef SUPPORT_OTA
        Serial.print(F("INIT: Starting OTA updater... "));
        if (WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.setPort(config.otaPort);
            ArduinoOTA.setHostname(config.hostname.c_str());
            ArduinoOTA.setPassword(config.otaPassword.c_str());
            ArduinoOTA.onStart([]() {
                Application::singleton->sysState = SystemState::UPDATING;
                // Handles start of OTA update. Determines update type.
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "sketch";
                }
                else {
                    type = "filesystem";
                }
                Serial.println("INFO: Starting OTA update (type: " + type + ") ...");
            });
            ArduinoOTA.onEnd([]() {
                // Handles update completion.
                Serial.println(F("INFO: OTA updater finished."));
            });
            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                // Reports update progress.
                ESPCrashMonitor.iAmAlive();
                Serial.printf("INFO: OTA Update Progress: %u%%\r", (progress / (total / 100)));
            });
            ArduinoOTA.onError([](ota_error_t error) {
                // Handles OTA update errors.
                Serial.printf("ERROR: OTA update error [%u]: ", error);
                switch(error) {
                    case OTA_AUTH_ERROR:
                        Serial.println(F("Auth failed."));
                        break;
                    case OTA_BEGIN_ERROR:
                        Serial.println(F("Begin failed."));
                        break;
                    case OTA_CONNECT_ERROR:
                        Serial.println(F("Connect failed."));
                        break;
                    case OTA_RECEIVE_ERROR:
                        Serial.println(F("Receive failed."));
                        break;
                    case OTA_END_ERROR:
                        Serial.println(F("End failed."));
                        break;
                }
            });

            #ifdef ENABLE_MDNS
                const bool useMDNS = true;
            #else
                const bool useMDNS = false;
            #endif

            ArduinoOTA.setMdnsEnabled(useMDNS);
            ArduinoOTA.begin();
            Serial.println(F("DONE"));
        }
        else {
            Serial.println(F("FAIL"));
        }
    #endif
}

void Application::initMQTT() {
    Serial.print(F("INIT: Initializing MQTT client... "));
    mqttClient.setBufferSize(256);
    mqttClient.setKeepAlive(45);
    mqttClient.setServer(config.mqttBroker.c_str(), config.mqttPort);
    mqttClient.setCallback(appOnMqttMessage);
    Serial.println(F("DONE"));
    if (reconnectMqttClient()) {
        delay(500);
        publishSystemState();
    }
}

void Application::initTimeclient() {
    Serial.print(F("INIT: Initializing NTP client... "));
    timeClient = new NTPClient(ntpUdp, NTP_POOL, config.clockTimezone * 3600);
    timeClient->begin();
    Serial.println(F("DONE"));
}

void Application::onCheckWiFi() {
    Serial.println(F("INFO: Checking WiFi connectivity..."));
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WARN: Lost connection. Attempting reconnect..."));
        connectWifi();
        if (WiFi.status() == WL_CONNECTED) {
            initMDNS();
            initOTA();
            initMQTT();
        }
    }
}

void Application::handleNewHostname(const char* newHostname) {
    config.hostname = newHostname;
    initMDNS();
}

void Application::handleSwitchToStatic(IPAddress newIp, IPAddress newSm, IPAddress newGw, IPAddress newDns) {
    config.ip = newIp;
    config.sm = newSm;
    config.gw = newGw;
    config.dns = newDns;
    Serial.println(F("INFO: Set static network config."));
    WiFi.config(config.ip, config.gw, config.sm, config.dns);
}

void Application::handleReconnectFromConsole() {
    // Attempt to reconnect to WiFi.
    onCheckWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        printNetworkInfo();
        //resumeNormal();
    }
    else {
        Serial.println(F("ERROR: Still no network connection."));
        Console.enterCommandInterpreter();
    }
}

void Application::handleWifiConfig(String newSsid, String newPassword) {
    config.ssid = newSsid;
    config.password = newPassword;
    connectWifi();
}

void Application::handleSaveConfig() {
    saveConfiguration();
    WiFi.disconnect(true);
    onCheckWiFi();
}

void Application::handleMqttConfigCommand(String newBroker, int newPort, String newUsername, String newPassw, String newConChan, String newStatChan) {
    mqttClient.unsubscribe(config.mqttTopicControl.c_str());
    mqttClient.disconnect();

    config.mqttBroker = newBroker;
    config.mqttPort = newPort;
    config.mqttUsername = newUsername;
    config.mqttPassword = newPassw;
    config.mqttTopicControl = newConChan;
    config.mqttTopicStatus = newStatChan;

    initMQTT();
    Serial.println();
}

void Application::handleBusResetCommand() {
    resetCommBus();
    initCommBus();
    initCoreIO();
}

void Application::initConsole() {
    Serial.print(F("INIT: Initializing console... "));

    Console.setHostname(config.hostname);
    Console.setMqttConfig(
        config.mqttBroker,
        config.mqttPort,
        config.mqttUsername,
        config.mqttPassword,
        config.mqttTopicControl,
        config.mqttTopicStatus
    );
    Console.onRebootCommand(doReboot);
    Console.onScanNetworks(appGetAvailableNetworks);
    Console.onFactoryRestore(appDoFactoryRestore);
    Console.onHostnameChange(appHandleNewHostname);
    Console.onDhcpConfig(appHandleSwitchToDhcp);
    Console.onStaticConfig(appHandleSwitchToStatic);
    Console.onReconnectCommand(appHandleReconnectFromConsole);
    Console.onWifiConfigCommand(appHandleWifiConfig);
    Console.onSaveConfigCommand(appHandleSaveConfig);
    Console.onMqttConfigCommand(appHandleMqttConfigCommand);
    Console.onBusReset(appHandleBusResetCommand);
}

void Application::init() {
    keypadQueue = xQueueCreate(5, sizeof(KeypadData));
    fobReaderQueue = xQueueCreate(5, sizeof(Tag));
    dryContactQueue = xQueueCreate(1, sizeof(uint8_t));
    optoContactQueue = xQueueCreate(1, sizeof(uint8_t));
	initSys();
	initCommBus();
	initCoreIO();
    heartbeatTask = initHeartbeat();
	initRelayModules();
	keypadCheckTask = initKeypadDevices();
    fobReaderCheckTask = initFobReaderDevices();
	initFilesystem();
    wifiCheckTask = initCheckWiFi();
    mqttCheckTask = initCheckMqtt();
    initMDNS();
    initOTA();
    clockSyncTask = initClockSync();
    inputTask = initInputTask();
    initConsole();
    sysState = SystemState::NORMAL;
    Serial.println(F("INIT: Boot sequence complete."));
    ESPCrashMonitor.enableWatchdog(ESPCrashMonitorClass::ETimeout::Timeout_2s);
}

void Application::update() {
    ESPCrashMonitor.iAmAlive();
    Console.checkInterrupt();
    #ifdef SUPPORT_OTA
        ArduinoOTA.handle();
    #endif
    mqttClient.loop();

    // TODO Keypad input could be a command to arm/disarm the system,
    // unlock a door, see certain statuses, etc. Unlike other
    // security/access control systems though (I'm looking at you, DSC...),
    // our keypads will NOT be used for system configuration. Configuration
    // should be perform eith via OTA config data updates, or via config messages
    // sent over MQTT or by the local serial console.
    KeypadData kpData;
    if (xQueueReceive(keypadQueue, &kpData, 0)) {
        onKeypadCommand(&kpData);
    }

    // TODO processing tags should be relatively simple:
    // Read each tag and verify if it is valid. If it *valid* then an appropriate
    // action should be taken (ie. firing a relay that controls a solenoid for
    // unlocking a door).
    Tag tag;
    if (xQueueReceive(fobReaderQueue, &tag, 0)) {
        onFobRead(&tag);
    }

    // TODO We should probably implement some kind of "reaction manager" so-to-speak to
    // handle how to react to these inputs.  This should involve some sort of mapping
    // that defines what should happen if an input is triggered. For example:
    // A REX attached to an input triggers a relay controlling a lock solenoid to unlock
    // a door. Or a door contact attached to an input triggers an alarm condition and
    // a siren goes off.
    uint8_t dcValue = 0;
    if (xQueueReceive(dryContactQueue, &dcValue, 0)) {
        // TODO What to do with the DC value?
    }

    uint8_t optoValue = 0;
    if (xQueueReceive(optoContactQueue, &optoValue, 0)) {
        // TODO What to do with the opto value?
    }
}