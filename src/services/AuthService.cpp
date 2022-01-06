#include "services/AuthService.h"
#include <HTTPClient.h>
#include "ArduinoJson.h"

AuthServiceClass::AuthServiceClass() {
	// TODO probably need to get the WiFiClient instance from main app.
}

void AuthServiceClass::setLoginEndpoint(const char* endpoint) {
	this->_loginEndpoint = endpoint;
}

void AuthServiceClass::setCardAuthEndpoint(const char* endpoint) {
	this->_cardAuthEndpoint = endpoint;
}

void AuthServiceClass::setPinAuthEndpoint(const char* endpoint) {
	this->_pinAuthEndpoint = endpoint;
}

void AuthServiceClass::setApiCredentials(String username, String password) {
	this->_username = username;
	this->_password = password;
}

String AuthServiceClass::login() {
	String result = "";
	Serial.println(F("INFO: [NET] Attempting to authenticate with API..."));
	if (WiFi.status() == WL_CONNECTED) {
		WiFiClient client;
		HTTPClient http;

		http.begin(client, this->_loginEndpoint);
		http.addHeader("Content-Type", "application/json");

		uint16_t freeMem = ESP.getFreeHeap() - 512;
        DynamicJsonDocument doc(freeMem);
		doc["username"] = this->_username;
		doc["password"] = this->_password;
		doc.shrinkToFit();

		String payload;
		serializeJson(doc, payload);
		doc.clear();

		int response = http.POST(payload.c_str());
		if (response == HTTP_CODE_OK) {
			freeMem = ESP.getFreeHeap() - 512;
			DynamicJsonDocument responsePayload(freeMem);
			DeserializationError err = deserializeJson(responsePayload, http.getString());
			if (err) {
				Serial.println(F("ERROR: [NET] Failed to parse JSON login response."));
			}
			else {
				responsePayload.shrinkToFit();
				result = responsePayload["token"].as<String>();
				Serial.print(F("INFO: [NET] Retrieved bearer token: "));
				Serial.println(result);
			}

			responsePayload.clear();
		}
		else {
			Serial.print(F("ERROR: [NET] Auth failed. Response code: "));
			Serial.println(response);
		}

		http.end();
	}
	else {
		Serial.println(F("ERROR: [NET] WiFi disconnected."));
	}

	return result;
}

bool AuthServiceClass::checkCardValid(const char* serial) {
	bool result = false;
	// TODO We need to NOT be retrieving a new auth token every time.
	// TODO the logic for login() should probably store the token if it is an
	// empty string, but if it isn't, then check the expiration and see if we
	// need to refresh the token first.
	String token = this->login();
	Serial.println(F("INFO: [NET] Checking card validity..."));
	if (token.length() == 0) {
		Serial.println(F("ERROR: [NET] API authorization failed."));
		return result;
	}

	if (WiFi.status() == WL_CONNECTED) {
		WiFiClient client;
		HTTPClient http;

		String url = this->_cardAuthEndpoint + String("?serial=") + String(serial);
		http.begin(client, url);
		http.addHeader("Content-Type", "application/json");
		http.setAuthorization("");
		http.addHeader("Authorization", "Bearer " + token);

		int response = http.GET();
		if (response == HTTP_CODE_OK) {
			uint16_t freeMem = ESP.getFreeHeap() - 512;
			DynamicJsonDocument responsePayload(freeMem);
			DeserializationError err = deserializeJson(responsePayload, http.getString());
			if (err) {
				Serial.println(F("ERROR: [NET] Failed to parse JSON login response."));
			}
			else {
				responsePayload.shrinkToFit();
				result = responsePayload["accepted"].as<bool>();
				Serial.print(F("INFO: [NET] Card validation success: "));
				Serial.println(result);
			}

			responsePayload.clear();
		}
		else {
			Serial.print(F("ERROR: [NET] Card validation failed. Response code: "));
			Serial.println(response);
		}

		http.end();
	}
	else {
		Serial.println(F("ERROR: [NET] WiFi disconnected."));
	}

	return result;
}

bool AuthServiceClass::checkPinValid(const char* pin) {
	bool result = false;
	String token = this->login();
	Serial.println(F("INFO: [NET] Checking pin validity..."));
	if (token.length() == 0) {
		Serial.println(F("ERROR: [NET] API authorization failed."));
		return result;
	}

	if (WiFi.status() == WL_CONNECTED) {
		WiFiClient client;
		HTTPClient http;

		String url = this->_pinAuthEndpoint + String("?pin=") + String(pin);
		http.begin(client, url);
		http.addHeader("Content-Type", "application/json");
		http.setAuthorization("");
		http.addHeader("Authorization", "Bearer " + token);

		int response = http.GET();
		if (response == HTTP_CODE_OK) {
			uint16_t freeMem = ESP.getFreeHeap() - 512;
			DynamicJsonDocument responsePayload(freeMem);
			DeserializationError err = deserializeJson(responsePayload, http.getString());
			if (err) {
				Serial.println(F("ERROR: [NET] Failed to parse JSON login response."));
			}
			else {
				responsePayload.shrinkToFit();
				result = responsePayload["accepted"].as<bool>();
				Serial.print(F("INFO: [NET] Pin validation success: "));
				Serial.println(result);
			}

			responsePayload.clear();
		}
		else {
			Serial.print(F("ERROR: [NET] Pin validation failed. Response code: "));
			Serial.println(response);
		}

		http.end();
	}
	else {
		Serial.println(F("ERROR: [NET] WiFi disconnected."));
	}

	return result;
}

AuthServiceClass AuthService;