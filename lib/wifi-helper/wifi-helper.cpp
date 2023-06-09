//#include <Arduino.h>
#include "wifi-helper.h"
 
/*
  Attempt to connect to wireless network ONCE, and wait ATTEMPT_DURATION (milliseconds) for 
  connection to be established. 
*/
bool connectWifi(const char *ssid, const char *passphrase)
{
  Sprint(F("Attempting to connect to wireless network \""));
  Sprint(ssid);
  Sprintln("\"... ");

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);
  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.    
  WiFi.begin(ssid, passphrase);   
  delay(500); // calls yield

  if(WiFi.status() == WL_CONNECTED){
    Sprintln(F("Wifi Connected!"));
    return true;
  }
  else{
    Sprintln(F("Wifi Failed!"));
    return false;
  }
}

/*
  Continuously tries to connect to the wireless network.
  Will wait ATTEMPT_COOLDOWN milliseconds between attempts. 

  WILL RESET IF UNABLE TO CONNECT TO NETWORK!

  Returns false if existing connection used, true if new connection established
  Will immediately return FALSE if already connected.
*/
bool assertNetworkConnectivity(const char *ssid, const char *passphrase){    

  if(WiFi.status() ==  WL_CONNECTED) {
    return false; // already connected, no new connection established
  }
  else {
    uint8_t failed_attempts = 0;
    do {
      if(!connectWifi(ssid, passphrase)){ // each attempt is allowed ATTEMPT_DURATION to establish a connection
        failed_attempts++;
        if(failed_attempts >= 10){
          // reset!
          ESP.restart();
        }
        else{
          Sprintln("COOLDOWN before wifi reconnection attempt...");
          delay(WIFI_ATTEMPT_COOLDOWN);
        }
      }    
    } 
    while (WiFi.status() != WL_CONNECTED);
    return true; // new connection established
  }
}

void printNetworkDetails()
{
  // SSID of the network you're attached to
  Sprint(F("SSID: "));
  Sprintln(WiFi.SSID()); // String

  // Hostname of this device
  Sprint(F("Hostname: "));
  Sprintln(WiFi.getHostname()); // char*

  Sprint(F("IP: "));
  Sprintln(WiFi.localIP()); // IPAddress

  Sprint(F("Subnet mask: "));
  Sprintln(WiFi.subnetMask()); // IPAddress

  Sprint(F("Broadcast IP: "));
  Sprintln(WiFi.broadcastIP()); // IPAddress

  Sprint(F("Gateway IP: "));
  Sprintln(WiFi.gatewayIP()); // IPAddress  

  Sprint(F("MAC: "));
  Sprintln(WiFi.macAddress());

  // MAC address of the router you're attached to
  Sprint("BSSID: ");
  Sprintln(WiFi.BSSIDstr()); // String

  // Received signal strength
  Sprint(F("RSSI: "));
  Sprintln(WiFi.RSSI()); // int8_t

  // Wifi password
  // Serial.print("PSK: ");
  // Serial.println(WiFi.psk()); // String

  // Encryption type:
  // byte encryption = WiFi.encryptionType();
  // Serial.print("Encryption Type: ");
  // Serial.println(encryption, HEX);

  // First DNS
  Sprint(F("DNS IP: "));
  Sprintln(WiFi.dnsIP(0)); // IPAddress

  Sprintln("");
}

std::string getMAC(){
  return std::string(WiFi.macAddress().c_str());
}

std::string getIP(){
  return std::string(WiFi.localIP().toString().c_str());
}

int getRSSI(){
  return WiFi.RSSI();
}