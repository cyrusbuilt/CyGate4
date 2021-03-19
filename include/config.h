#ifndef _CONFIG_H
#define _CONFIG_H

#include <IPAddress.h>

#define DEBUG
#define SUPPORT_OTA
#define SUPPORT_MDNS
#define DEFAULT_SSID "your_ssid_here"
#define DEFAULT_PASSWORD "your_password_here"
#define DEFAULT_TIMEZONE -4
#define SERIAL_BAUD 115200
#define CONFIG_FILE_PATH "/config.json"
#define DOOR_FILE_PATH "/doors.json"
#define CHECK_WIFI_INTERVAL 30000               // How often to check WiFi status (milliseconds).
#define CHECK_MQTT_INTERVAL 35000               // How often to check connectivity to the MQTT broker.
#define CLOCK_SYNC_INTERVAL 3600000             // How often to sync the local clock with NTP (milliseconds).
#define MQTT_TOPIC_STATUS "cygate4/status"
#define MQTT_TOPIC_CONTROL "cygate4/control"
#define MQTT_BROKER "your_mqtt_host_here"
#define MQTT_PORT 1883
#define DEFAULT_HOST_NAME "CYGATE4"
#define NTP_POOL "pool.ntp.org"
#ifdef SUPPORT_OTA
	#include <ArduinoOTA.h>
	#define DEFAULT_OTA_HOST_PORT 8266
	#define DEFAULT_OTA_PASSWORD "your_ota_password_here"
#endif

typedef struct {
    // Network stuff
    String hostname;
    String ssid;
    String password;
    IPAddress ip;
    IPAddress gw;
    IPAddress sm;
    IPAddress dns;
    bool useDhcp;
	bool mdnsEnable;

    uint8_t clockTimezone;

    // MQTT stuff
    String mqttTopicStatus;
    String mqttTopicControl;
    String mqttBroker;
    String mqttUsername;
    String mqttPassword;
    uint16_t mqttPort;

    // OTA stuff
	bool otaEnable;
    uint16_t otaPort;
    String otaPassword;
} config_t;

#endif