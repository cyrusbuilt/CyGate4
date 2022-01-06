#ifndef _AUTH_SERVICE_H
#define _AUTH_SERVICE_H

#include <Arduino.h>

class AuthServiceClass {
public:
	AuthServiceClass();
	void setLoginEndpoint(const char* endpoint);
	void setCardAuthEndpoint(const char* endpoint);
	void setPinAuthEndpoint(const char* endpoint);
	void setApiCredentials(String username, String password);
	bool checkCardValid(const char* serial);
	bool checkPinValid(const char* pin);

private:
	String login();

	const char* _loginEndpoint;
	const char* _cardAuthEndpoint;
	const char* _pinAuthEndpoint;
	String _username;
	String _password;
};

extern AuthServiceClass AuthService;

#endif