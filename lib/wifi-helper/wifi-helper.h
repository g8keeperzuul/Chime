#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <string>
#include <WiFi.h>
#include "esp_wifi.h"
#include "log.h"

#define WIFI_ATTEMPT_COOLDOWN 30000 // milliseconds between connection attempts

// *********************************************************************************************************************
// *** Must Declare ***
extern WiFiClient wificlient;

// *** Must Implement ***
bool connectWifi(const char *ssid, const char *passphrase);
bool assertNetworkConnectivity(const char *ssid, const char *passphrase);
void printNetworkDetails();
std::string getMAC();
std::string getIP();
int getRSSI();

#endif