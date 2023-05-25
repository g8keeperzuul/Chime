/*
  *** Siren Integration ***

  Play tone on command.

  https://www.home-assistant.io/integrations/siren/
  https://developers.home-assistant.io/docs/core/entity/siren


  homeassistant/siren/featheresp32s2/command   <<< set state; device will subscribe to this topic
  homeassistant/siren/featheresp32s2/state     >>> get state; device will publish to this topic

  Both command and state receive or reture the same payload:
  {
    state: ON
    volume_level: 0.0..1.0,
    tone: [alarm.mp3|doorbell.wav],  FILE EXPECTED TO BE FOUND IN DIRECTORY TONE_DIR ON SDCARD
    duration: 0..n   NOT SUPPORTED
  }  
  OR
  {
    state: OFF
  }  

*** Text Integration ***

Display message on command. 

homeassistant/text/featheresp32s2/display/command <<< {"text":"Basement smoke detector triggered!", "graphic": "FIRE" }
homeassistant/text/featheresp32s2/display/command <<< {"text":"hello world", "graphic": "NONE" }  <<< "hello world" input from HA

homeassistant/text/featheresp32s2/display/state   >>> {"text":"Basement smoke detector triggered!", "graphic": "FIRE" }
homeassistant/text/featheresp32s2/display/state   >>> {"text":"hello world", "graphic": "NONE" }

To clear the display:
homeassistant/text/featheresp32s2/display/command <<< {"text":"", "graphic": "NONE" }
homeassistant/text/featheresp32s2/display/command <<< ""

The first is preferrable since it more clearly reflects the command back to state topic.
The second will actually result in the command and state topics begin deleted in the broker.

Supported Graphics:
NONE, DOOR, RADS, FIRE, INFO, AQI, CO2, WATER, GAS, FREEZE, ALERT, SAFE, GARAGE

*** Diagnostics ***

Publishes diagnostic information according to refresh frequency.

homeassistant/siren/featheresp32s2/diagnostics >>> { "wifi_rssi": -43, "wifi_ip": "10.0.0.177", "wifi_mac": "84:F7:03:D6:8B:20" }

*** Configuration *** 

Publication frequency in minutes.

homeassistant/number/featheresp32s2/refreshrate/get >>> 1
homeassistant/number/featheresp32s2/refreshrate/set <<< 1


********************************************************************************
DISCOVERY CONFIGURATION.YAML:

This is what the manual configuration for siren and text would look like. 
However it is unnecessary as the device sends auto-discovery MQTT messages upon startup.

mqtt:
  siren:
    - unique_id: featheresp32s2_chime
      name: "Chime"
      icon: "mdi:bullhorn"
      device: 
        name: "Chime"
        identifiers: "84:F7:03:D6:8B:20"
        mf: "Adafruit"
        mdl: "Feather ESP32-S2 with BME280"
        sw: "20230302.2100"      

      availability_topic: "homeassistant/siren/featheresp32s2/availability"
      
      state_topic: "homeassistant/siren/featheresp32s2/state"        
      state_value_template: "{{ value_json.state }}"
      state_on: "ON"
      state_off: "OFF"

      command_topic: "homeassistant/siren/featheresp32s2/command"  
      command_template: "{ 'state': '{{ value }}', 'tone': '{{ tone }}', 'volume_set': {{ volume_level }}, 'duration': {{ duration }} }"
      command_off_template": "{ 'state': '{{ value }}' }"
      payload_on: "ON"
      payload_off: "OFF"

      available_tones:
        - doorbell
        - alarm
      support_duration: true
      support_volume_set: true
      
      optimistic: false
      qos: 0
      retain: true

  mqtt:
    text:
      - unique_id: "featheresp32s2_display"
        name: "Chime display"
        icon: "mdi:image-text"
        min: 0
        max: 255
        availability_topic: "homeassistant/siren/featheresp32s2/availability"
        state_topic: "homeassistant/text/featheresp32s2/display/state"
        value_template: "{{ value_json.text }}"
        command_topic: "homeassistant/text/featheresp32s2/display/command"
        command_template: "{ 'text': '{{ value }}', 'graphic': 'NONE' }"
        optimistic: false
        device:
          name: "Chime"
          identifiers: "84:F7:03:D6:8B:20"
          mf: "Adafruit"
          mdl: "Feather ESP32-S2 with BME280"
          sw: "20230302.2100"      
*/

#include "chime.h"
#include "graphics.h"

// Time
const char* ntpServer = NTP_SERVER;
RTC_DATA_ATTR bool time_is_set = false;
char lastboot[80];  // yyyy-mm-ddThh:mm:ss[+|-]zzz  ISO8601

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL);

Adafruit_BME280 bme; // I2C

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// 128x128 SSD1327 display connected via I2C
Adafruit_SSD1327 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET, 1000000);
// software SPI
//Adafruit_SSD1327 display(128, 128, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// hardware SPI
//Adafruit_SSD1327 display(128, 128, &SPI, OLED_DC, OLED_RESET, OLED_CS);

WiFiClient wificlient;
MQTTClient mqttclient(768); // default is 128 bytes;  https://github.com/256dpi/arduino-mqtt#notes

unsigned long refresh_rate = 60000; // 1 minutes default; frequency of sensor updates in milliseconds

/*
  Since the fully constructed list of discovery_config (topics and payloads) consumes considerable RAM, reduce it to just the facts.
  Generate each discovery_config one at a time at the point of publishing the message in order to conserve RAM.
  Once everything is successfully published, purge list contents (no longer needed).
*/
std::vector<discovery_metadata> discovery_metadata_list;                                            // list of data used to construct discovery_config for sensor discovery
std::vector<discovery_config_metadata> discovery_config_metadata_list;                              // list of data used to construct discovery_config for config/control discovery
std::vector<discovery_measured_diagnostic_metadata> discovery_measured_diagnostic_metadata_list;    // list of data used to construct discovery_config for sensor measured diagnostics (like RSSI)
std::vector<discovery_fact_diagnostic_metadata> discovery_fact_diagnostic_metadata_list;            // list of data used to construct discovery_config for fact diagnostics (like IP address)

// Last will and testament topic
const std::string AVAILABILITY_TOPIC = buildAvailabilityTopic("siren", std::string(DEVICE_ID)); // homeassistant/siren/featheresp32s2/availability

const std::string DIAGNOSTIC_TOPIC = buildDiagnosticTopic("siren", std::string(DEVICE_ID)); // homeassistant/siren/featheresp32s2/diagnostics

// All sensor updates are published in a single complex json payload to a single topic
const std::string STATE_TOPIC = buildStateTopic("siren", std::string(DEVICE_ID)); // homeassistant/siren/featheresp32s2/state

const std::string ON_VALUE = "ON";
const std::string OFF_VALUE = "OFF";

// *****************************

std::vector<std::string> availableTones(File dir){
  std::vector<std::string> tones = { }; 

  Sprintln("Found tones:");
  while(true) {     
     File entry =  dir.openNextFile();
     if(!entry) {
       // no more files       
       break;
     }
     if(!entry.isDirectory()){
      Sprint("\t"); Sprintln(entry.name());
      tones.push_back(entry.name());
     }   
     entry.close();
   }
  
  return tones;
}

/*
  device_class : https://developers.home-assistant.io/docs/core/entity/sensor?_highlight=device&_highlight=class#available-device-classes
                 https://www.home-assistant.io/integrations/sensor/#device-class
  has_sub_attr : true if you want to provide sub attributes under the main attribute
  icon : https://materialdesignicons.com/
  unit : (depends on device class, see link above)
*/
// build discovery message - step 1 of 4
std::vector<discovery_metadata> getAllDiscoveryMessagesMetadata(){
/*
  NOTE: SENSOR INACCURACY!
  
  These sensor are available as they are included with the BME280.
  However, the temperature (and hence humidity) sensors are wildly abnormal due to their proximity to the CPU.
  They can only be used during brief periods shortly after waking from >=5min sleep in order to obtain an accurate reading. 

  discovery_metadata temp, humidity, pressure;

  // default device_type = "sensor"
  // default sub_attr = false

  temp.device_class = "temperature";  
  temp.icon = "mdi:home-thermometer";
  temp.unit = "°C";

  humidity.device_class = "humidity";  
  humidity.icon = "mdi:water-percent";
  humidity.unit = "%";

  pressure.device_class = "pressure";
  pressure.icon = "mdi:gauge";
  pressure.unit = "hPa";
*/
  std::vector<discovery_metadata> dm = { /*temp, humidity, pressure*/ };  
  return dm;
}

// build discovery config/control message - step 1 of 4
std::vector<discovery_config_metadata> getAllDiscoveryConfigMessagesMetadata(){
  discovery_config_metadata refrate, siren, display;

  /*
  Dynamically build list of available tones from all files in /tones directory on SD card.
  */
  std::vector<std::string> tones = availableTones(SD.open(TONE_DIR));
  std::string custom_settings = "\"optimistic\": false, \"support_duration\": false, \"support_volume_set\": true, \"available_tones\": [";  
  bool first = true;
  for(int i=0;i < tones.size(); i++){
    if(first){
      first = false;
    }
    else{
      custom_settings.append(",");
    }
    custom_settings.append("\"");
    custom_settings.append(tones.at(i));
    custom_settings.append("\"");
  }
  custom_settings.append("]");

  siren.device_type = "siren";
  siren.control_name = "chime";
  //siren.custom_settings = "\"optimistic\": false, \"support_duration\": false, \"support_volume_set\": true, \"available_tones\": [\"doorbell.wav\", \"alarm.mp3\"]";
  siren.custom_settings = custom_settings;
  siren.icon = "mdi:bullhorn";
  siren.unit = ""; 
  // override default get/set topic names
  siren.set_topic = "homeassistant/siren/featheresp32s2/command";
  siren.get_topic = "homeassistant/siren/featheresp32s2/state";


  refrate.device_type = "number";
  refrate.control_name = "refreshrate";
  refrate.custom_settings = "\"min\": 1, \"max\": 60, \"step\": 1";
  refrate.icon = "mdi:refresh-circle";
  refrate.unit = "minutes";
  // default topic names if not specified
  //refrate.set_topic = "homeassistant/number/featheresp32s2/refreshrate/set";
  //refrate.get_topic = "homeassistant/number/featheresp32s2/refreshrate/get";

  display.device_type = "text";
  display.control_name = "display";
  display.custom_settings = "\"min\": 0, \"max\": 84"; // 255 is the max
  display.icon = "mdi:image-text";
  display.unit = "";
  // override default get/set topic names
  display.set_topic = "homeassistant/text/featheresp32s2/display/command"; // command payload= { "text": "Basement smoke detector triggered!", "graphic": "FIRE" }
  display.get_topic = "homeassistant/text/featheresp32s2/display/state"; // device reflects command payload to the state topic (same as above)
  display.custom_settings = "\"command_template\": \"{ 'text': '{{ value }}', 'graphic': 'NONE' }\", \"value_template\": \"{{ value_json.text }}\""; // need to extract value of text attribute in order to be compatible with "text" device type

  std::vector<discovery_config_metadata> dcm = { refrate, siren, display };  
  return dcm;
}

std::vector<discovery_measured_diagnostic_metadata> getAllDiscoveryMeasuredDiagnosticMessagesMetadata(){
  discovery_measured_diagnostic_metadata rssi;

  rssi.device_type = "sensor";
  rssi.device_class = "";  // battery | date | duration | timestamp | ... In some cases may be "None" - https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes    
  //rssi.state_class = "measurement";  Accept default
  rssi.diag_attr = "wifi_rssi";
  rssi.icon = "mdi:wifi-strength-2";  // https://materialdesignicons.com/
  rssi.unit = ""; // RSSI is unitless
  
  std::vector<discovery_measured_diagnostic_metadata> dmdm = { rssi };  
  return dmdm;
}

std::vector<discovery_fact_diagnostic_metadata> getAllDiscoveryFactDiagnosticMessagesMetadata(){
  discovery_fact_diagnostic_metadata ip, mac, lastboot;
  
  ip.device_type = "sensor";
  ip.diag_attr = "wifi_ip";
  ip.icon = "mdi:ip-network";  

  mac.device_type = "sensor";
  mac.diag_attr = "wifi_mac";
  mac.icon = "mdi:network-pos";  

  lastboot.device_type = "sensor";
  lastboot.diag_attr = "last_boot";
  lastboot.icon = "mdi:clock-start";    

  std::vector<discovery_fact_diagnostic_metadata> dfdm = { ip, mac, lastboot };  
  return dfdm;
}

/* If the buildShortDevicePayload() is used, and there are no config/controls that use the full device payload, then HA will never see it.
   So the full device payload is used for both kinds of discovery message. The short version is used for diagnostic messages because they 
   are always accompanied by sensor and/or control messages. */

// build discovery message - step 3 of 4
discovery_config getDiscoveryMessage(discovery_metadata disc_meta){
    return getDiscoveryMessage(disc_meta, DEVICE_ID, buildDevicePayload(DEVICE_NAME, getMAC(), DEVICE_MANUFACTURER, DEVICE_MODEL, DEVICE_VERSION), AVAILABILITY_TOPIC, STATE_TOPIC);
}

// build discovery config/control message - step 3 of 4
discovery_config getDiscoveryMessage(discovery_config_metadata disc_meta){  
  return getDiscoveryConfigMessage(disc_meta, DEVICE_ID, buildDevicePayload(DEVICE_NAME, getMAC(), DEVICE_MANUFACTURER, DEVICE_MODEL, DEVICE_VERSION), AVAILABILITY_TOPIC);
}

// build discovery measured diagnostic message - step 3 of 4
discovery_config getDiscoveryMessage(discovery_measured_diagnostic_metadata disc_meta){  
  return getDiscoveryMeasuredDiagnosticMessage(disc_meta, DEVICE_ID, buildShortDevicePayload(DEVICE_NAME, getMAC()), AVAILABILITY_TOPIC, DIAGNOSTIC_TOPIC);
}

// build discovery diagnostic fact message - step 3 of 4
discovery_config getDiscoveryMessage(discovery_fact_diagnostic_metadata disc_meta){  
  return getDiscoveryFactDiagnosticMessage(disc_meta, DEVICE_ID, buildShortDevicePayload(DEVICE_NAME, getMAC()), AVAILABILITY_TOPIC, DIAGNOSTIC_TOPIC);
}

// *****************************

/*
  Using text size = 1, fits 336 chars (21 chars over 16 lines)
  Using text size = 2, fits 80 chars (10 chars over 8 lines)
  Use '\n' to insert linefeed in string.
*/
void displayMessage(uint8_t size, const char* message){

    display.clearDisplay();

    display.setTextSize(size);
    display.setTextColor(SSD1327_WHITE);
    display.setCursor(0,0);
    display.println(message);

    display.display();
}

/*
  Title should be <= 10 chars
*/
void displayMessage(const char* title, const char* message){
    display.clearDisplay();
    
    display.setTextSize(2); // 12x16
    display.setTextColor(SSD1327_BLACK, SSD1327_WHITE); // 'inverted' text
    display.setCursor(0,0);
    display.print(title);

    display.setTextSize(1); // 6x8
    display.setTextColor(SSD1327_WHITE);    
    display.setCursor(0,17);    
    display.println(message);  

    display.display();
}

/*  
  Message will be displayed in size 1. 
  Up to 84 chars (4 full lines) can be displayed under a 96x96 graphic.
  Graphic will be centered and top-justified.

  Usage:
  displayGraphic("ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTU", FIRE_BITMAP, FIRE_WIDTH, FIRE_HEIGHT);
*/
void displayGraphic(const char* message, const u_int8_t* bitmap, uint16_t bitmap_w, uint16_t bitmap_h){  
  display.clearDisplay();
  display.drawBitmap((int)((DISPLAY_WIDTH-bitmap_w)/2), 0,  bitmap, bitmap_w, bitmap_h, SSD1327_WHITE);  

  if(strlen(message)>0){ // ensure message is not null
    display.setTextSize(1); // 6x8
    display.setTextColor(SSD1327_WHITE);    
    display.setCursor(0,bitmap_h); // start text left justified under graphic   
    display.println(message);  
  }

  display.display();    
}

void displayClear(){  
  display.clearDisplay();
  display.display();    
}

void displayWifiOffline(){
  displayGraphic("Unable to connect to network!", WIFI_ALERT_BITMAP, WIFI_ALERT_WIDTH, WIFI_ALERT_HEIGHT);    
}

void displayMQTTOffline(){
  displayGraphic("Unable to connect to MQTT broker!", MQTT_ALERT_BITMAP, MQTT_ALERT_WIDTH, MQTT_ALERT_HEIGHT);    
}

void displayBroken(const char *message){
  displayGraphic(message, BROKEN_BITMAP, BROKEN_WIDTH, BROKEN_HEIGHT);    
}


/*
  This will begin playing the tone.
  With useInterrupts() functional, a hardware interrupt will cause the automatic invocation of feedBuffer().
  However, without this hardware interrupt, feedBuffer() must be explicitly called until musicPlayer.playingMusic is FALSE (see loop()).
*/
void activateSiren(const char *tone, float volume_level, int duration /*ignored*/){  
  Sprintln(F("Activate siren!"));
  Sprint("tone = "); Sprintln(tone);
  Sprint("volume_level = "); Sprintln(volume_level);
  Sprint("duration = "); Sprintln(duration);

  // VOLUME
  // Set volume for left, right channels. lower numbers == louder volume!
  uint8_t vol = 0;
  if(volume_level > 0.0){
    vol = ceil(255 * volume_level); // 1..255  where 1 is quietest, 255 loudest
    vol = 256 - vol; // 1..255  where 1 is loudest, 255 is quietest
  }  
  musicPlayer.setVolume(vol,vol);  
  Sprint("vol = "); Sprintln(vol);

  // DURATION
  //?
  
  /*
  Since the tone names are the actual filenames (restricted to 8.3 on FAT formatted SDcard), all 
  this does is prepends the directory path where the tones are stored. 

  alarm.mp3 --> /tones/alarm.mp3
  */
  String filename = TONE_DIR + String("/") + String(tone); // /tones/alarm.mp3  
     
  Sprint(F("Playing file: ")); Sprintln(filename);

  // This will starve the main loop() until the tone is completely played.
  // if(musicPlayer.playFullFile(filename)){
  //   Sprintln("play end");
  // }
  if(!musicPlayer.startPlayingFile(filename.c_str())){    
    Sprintln(F("Failed to play!"));
  }  
}


void deactivateSiren(){  
  Sprintln(F("Deactivate siren"));

  if(!musicPlayer.stopped()){
    musicPlayer.stopPlaying();
  }
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}

void indicateSensorProblem(byte return_code){
  Sprintln(F("ERROR: Sensor problem!"));
  // ten "short" flashes for 0.5s each
  int count = 0;
  do {
      pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red
      pixels.show();   // Send the updated pixel colors to the hardware.
      delay(500); 
  } 
  while(count < 10);  
  pixels.clear();
}

// https://stackoverflow.com/questions/4668760/converting-an-int-to-stdstring
std::string to_string( int x ) {
  int length = snprintf( NULL, 0, "%d", x );
  assert( length >= 0 );
  char* buf = new char[length + 1];
  snprintf( buf, length + 1, "%d", x );
  std::string str( buf );
  delete[] buf;
  return str;
}

/**
 * @brief Convert a float to a std::string
 * 
 * @param x float to be formatted and converted to std::string
 * @param format Should be related to float like "%4.2f" or "%3.1g"
 * @return std::string 
 */
std::string to_string( float x, const char* format) {
  int length = snprintf( NULL, 0, format, x );
  assert( length >= 0 );
  char* buf = new char[length + 1];
  snprintf( buf, length + 1, format, x );
  std::string str( buf );
  delete[] buf;
  return str;
}

/*
  temperature                         : sensor on BME280 in celcius
  humidity                            : sensor on BME280 in %RH
  pressure                            : sensor on BME280 is atmospheric pressure in hectopascal (hPa)
*/
void publishSensorData(){
  
      std::string payload = "{\
\"temperature\": "+to_string(bme.readTemperature(), "%2.2f")+", \
\"humidity\": "+to_string(bme.readHumidity(), "%2.2f")+", \
\"pressure\": "+to_string((bme.readPressure() / 100.0F), "%5.1f")+" \
}";

      const char* payload_ch = payload.c_str();

      Sprint(F("Publishing sensor readings: "));  
      Sprintln(payload_ch);
      mqttclient.publish(STATE_TOPIC.c_str(), payload_ch, NOT_RETAINED, QOS_0);  
}

void publishDiagnosticData(){  

  std::string payload = "{\
\"wifi_rssi\": "+to_string(getRSSI())+", \
\"wifi_ip\": \""+getIP()+"\", \
\"wifi_mac\": \""+getMAC()+"\", \
\"last_boot\": \""+lastboot+"\" \
}";

  const char* payload_ch = payload.c_str();

  Sprint(F("Publishing diagnostic readings: "));  
  Sprint(DIAGNOSTIC_TOPIC.c_str()); Sprint(F(" : "));
  Sprintln(payload_ch);

  mqttclient.publish(DIAGNOSTIC_TOPIC.c_str(), payload_ch, NOT_RETAINED, QOS_0);   
}

/*
  There are config/controls that provide their own getter topic instead of the state or diagnostic topic.
  These topics will reflect the requested (via setter topic) update when a control/config change is made.
  However, they need to be periodically refreshed to keep the Home Assistant UI up-to-date even when a control isn't changed.

  homeassistant/number/featherm0/volume/get
  homeassistant/number/featherm0/refreshrate/get  
*/
void publishConfigData(){
 for (size_t i = 0; i < discovery_config_metadata_list.size(); i++){
    String test_control_name = String(discovery_config_metadata_list[i].control_name.c_str());
    if(test_control_name.equals("refreshrate")){
      publish(discovery_config_metadata_list[i].get_topic.c_str(), String(refresh_rate/1000/60));
    }
    // The chime control state is actually updated when a command is received and when the player naturally plays to the end of the audio file.
    else if(test_control_name.equals("chime")){
      // no need to update state during refresh. In fact only the state of the player is global, not the volume or tone details.
      //publish(discovery_config_metadata_list[i].get_topic.c_str(), String("{\"state\":\"ON|OFF\""}"));
    } 
    else if(test_control_name.equals("display")){      
      // TODO: Update display state during refresh cycle
      //publish(discovery_config_metadata_list[i].get_topic.c_str(), String("{\"text\":\")+String(current_text)+String("\", \"graphic\": \")+String(current_graphic)"\""}"));
    }        
    else{
      Sprint(F("WARN: Config/control state not published: ")); Sprintln(test_control_name);
    }     
  }
}

std::queue<pending_config_op> pending_ops = {};

/* 
Processes pending operations on the pending_ops queue.
*/
void processMessages(){
  while(!pending_ops.empty()){
    pending_config_op op = pending_ops.front();
    Sprint(F("Processing pending message : ")); Sprintln(op.config_meta.control_name.c_str());
    
    if(op.config_meta.control_name.compare("chime") == 0){
      /*
        {
          state: ON,
          volume_level: 0.0..1.0,
          tone: [alarm.mp3|doorbell.wav|...],
          duration: 0..n     NOT SUPPORTED 
        }  
        OR
        {
          state: OFF
        }       
      */
      
      // Inside the brackets is the capacity of the memory pool in bytes.      
      // Use https://arduinojson.org/v6/assistant to compute the capacity.
      StaticJsonDocument<128> doc;

      DeserializationError error = deserializeJson(doc, op.value);

      if(error) {
        Sprint(F("Failed to parse command: "));
        Sprintln(error.c_str());       
      }
      else
      {
        const char* state = doc["state"];           // "ON" | "OFF"

        if(ON_VALUE.compare(state) == 0){
          
          // the following parameters will only be present if the state is active (ON)
          //const char* volume_set_ch = doc["volume_set"];   // 0.0..1.0
          //Sprint("volume_set = "); Sprintln(volume_set_ch);
          //float volume_set=atof(volume_set_ch);          

          float volume_set=doc["volume_set"].as<float>();   // 0.0..1.0
          //Sprint("volume_set = "); Sprintln(volume_set);          

          const char* tone = doc["tone"];             // "doorbell.wav" | "alarm.mp3"  (only pre-defined tones will be allowed by Home Assistant)
          //int duration = doc["duration"];           // 999    
          int duration = -1;

          activateSiren(tone, volume_set, duration);
        }
        else{          
          deactivateSiren();
        }        
        // publish updated value - reflects command payload as state update
        publish(op.config_meta.get_topic.c_str(), op.value);
      }
      // once processed, remove from queue
      pending_ops.pop(); // deletes from front
    }
    else if(op.config_meta.control_name.compare("refreshrate") == 0){
    
      int rr = op.value.toInt();
      if(rr < 1){ rr = 1; }
      if(rr > 60){ rr = 60; }
      
      refresh_rate = rr * 60 * 1000; // rr (minutes) --> refresh_rate (milliseconds)

      // publish updated value
      publish(op.config_meta.get_topic.c_str(), String(rr));

      // once processed, remove from queue
      pending_ops.pop(); // deletes from front
    }  
    else if(op.config_meta.control_name.compare("display") == 0){

      if(op.value.isEmpty()){ // if command is empty, clear display
        display.clearDisplay();
        display.display();
      }
      else{ // command is not empty...            
        // Inside the brackets is the capacity of the memory pool in bytes.      
        // Use https://arduinojson.org/v6/assistant to compute the capacity.
        StaticJsonDocument<128> doc;        
        DeserializationError error = deserializeJson(doc, op.value);

        if(error) {
          Sprint(F("Failed to parse command due to: "));
          Sprintln(error.c_str()); 
          Sprintln(op.value);
        }
        else
        {        
          const char* text = doc["text"]; // up to 84 characters
          const char* graphic = doc["graphic"]; // up to 15 characters mapping to graphic name   

          Sprint("display graphic: "); Sprintln(graphic);       
          Sprintln(text);

          // map requested "graphic" to matching bitmap
          if(strcmp(graphic, NONE_LABEL)==0){ 
            displayMessage(1,text);
          }  
          else if(strcmp(graphic, MEDIUM_TEXT_LABEL)==0){
            displayMessage(2,text);
          }            
          else if(strcmp(graphic, LARGE_TEXT_LABEL)==0){
            displayMessage(3,text);
          }                   
          else if(strcmp(graphic, AQI_LABEL)==0){ 
            displayGraphic(text, AQI_BITMAP, AQI_WIDTH, AQI_HEIGHT);
          }
          else if(strcmp(graphic, FIRE_LABEL)==0){
            displayGraphic(text, FIRE_BITMAP, FIRE_WIDTH, FIRE_HEIGHT);
          }
          else if(strcmp(graphic, CO2_LABEL)==0){
            displayGraphic(text, CO2_BITMAP, CO2_WIDTH, CO2_HEIGHT);
          } 
          else if(strcmp(graphic, RADIATION_LABEL)==0){
            displayGraphic(text, RADIATION_BITMAP, RADIATION_WIDTH, RADIATION_HEIGHT);
          }            
          else if(strcmp(graphic, DOORBELL_LABEL)==0){
            displayGraphic(text, DOORBELL_BITMAP, DOORBELL_WIDTH, DOORBELL_HEIGHT);
          }  
          else if(strcmp(graphic, GARAGE_LABEL)==0){
            displayGraphic(text, GARAGE_BITMAP, GARAGE_WIDTH, GARAGE_HEIGHT);
          }                             
          else if(strcmp(graphic, SAFETY_LABEL)==0){
            displayGraphic(text, SAFETY_BITMAP, SAFETY_WIDTH, SAFETY_HEIGHT);
          } 
          else if(strcmp(graphic, ALERT_LABEL)==0){
            displayGraphic(text, ALERT_BITMAP, ALERT_WIDTH, ALERT_HEIGHT);
          } 
          else if(strcmp(graphic, FREEZE_LABEL)==0){
            displayGraphic(text, FREEZE_BITMAP, FREEZE_WIDTH, FREEZE_HEIGHT);
          } 
          else if(strcmp(graphic, GAS_LABEL)==0){
            displayGraphic(text, GAS_BITMAP, GAS_WIDTH, GAS_HEIGHT);
          }               
          else if(strcmp(graphic, WATER_LABEL)==0){
            displayGraphic(text, WATER_BITMAP, WATER_WIDTH, WATER_HEIGHT);
          }                                                 
          else{ // default is INFO graphic if no match found
            displayGraphic(text, INFO_BITMAP, INFO_WIDTH, INFO_HEIGHT);
            // graphic = "INFO"
          }
          // TODO: Support refresh of display state. 
          // Record for state refresh
          // current_text = text;
          // current_graphic = graphic
        }
      }
      
      // publish updated value - reflects display set value as get value
      publish(op.config_meta.get_topic.c_str(), op.value);

      // once processed, remove from queue
      pending_ops.pop(); // deletes from front      
    }      
    else{
      // operation ignored; delete from queue anyway to void endless loop
      Sprint(F("Message ignored! : ")); Sprintln(op.config_meta.control_name.c_str());
      pending_ops.pop();
    }
  }
  
}

void publish(){
    publishOnline(AVAILABILITY_TOPIC.c_str());
    //publishSensorData();    
    publishDiagnosticData();
    publishConfigData();
}

bool onMQTTConnect(){

  Sprintln("ENTER >>> onMQTTConnect()");
  displayClear(); // erase any wifi offline or broker disconnected displayed messages
  //print_heap();

  if(subscribeTopics(getAllSubscriptionTopics(std::string(DEVICE_ID)))){  
    // Must successfully publish all discovery messages before proceding 
    int discovery_messages_pending_publication;
    do {
      discovery_messages_pending_publication = publishDiscoveryMessages(); // Create the discovery messages and publish for each topic. Update published flag upon successful publication.
    }
    while(discovery_messages_pending_publication != 0);
  
    // no longer need discovery metadata, so purge it from memory
    //purgeDiscoveryMetadata();

    //print_heap();

    // since the loop() won't publish until the refresh rate has been triggered,
    // begin with an immediate publication upon connect with the MQTT broker 
    // (since refresh rate could be 1 hour).
    publish();
    
    Sprintln("EXIT >>> onMQTTConnect()");    
    return true;
  }
  else {
    // if there is a problem with subscribing to a topic or publishing discovery messages then disconnect from the broker and try again    
    Sprintln(F("Failed to setup with MQTT broker!"));
    mqttclient.disconnect();  
    
    return false;
  }
   
}

void setClock(){
    Sprintln("Setting clock...");
    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  
    // Just set UTC time since timezone info is lost during deep sleep (getLocalTime() was returning UTC)
    configTime(0, 0, ntpServer);  
  
    // set timezone before querying time
    int rcode = setenv("TZ", TIMEZONE, 1);
    tzset();  

    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){      
      //lastboot = asctime(&timeinfo);      
      strftime(lastboot,80,"%FT%T%z",&timeinfo);  // yyyy-mm-ddThh:mm:ss[+|-]zzz  ISO8601
      Sprint("Boot timestamp: "); Sprintln(lastboot);
    }
}


// medium blue while attempting to connect to wifi
bool assertNetworkConnectivity(){
  
  if(WiFi.status() !=  WL_CONNECTED) {
    // Only show blue when attempting to connect to wifi
    pixels.setPixelColor(0, pixels.Color(0, 0, 128)); // medium blue
    pixels.show();   
    displayWifiOffline(); // will remain displayed until network actually connects; unfortunately it means it may flash briefly when initially connecting in normal circumstances.
    bool is_new_connection = assertNetworkConnectivity(LOCAL_ENV_WIFI_SSID, LOCAL_ENV_WIFI_PASSWORD);
    pixels.clear(); 
    pixels.show();     
    return is_new_connection;
  }
  else{
    return false; // existing connection still intact
  }
}

bool mqtt_initialized = false;

/*
  Will execute (with cooldown between attempts) until connected to MQTT broker successfully, and successfully subscribed.
*/
void assertMQTTConnectivity(){
  Sprintln("ENTER >>> assertMQTTConnectivity()");  
  pixels.setPixelColor(0, pixels.Color(128, 0, 128)); // medium purple
  pixels.show(); 
  displayMQTTOffline(); // will remain displayed until device actually connects to MQTT broker; unfortunately it means it may flash briefly when initially connecting in normal circumstances.

  if(!mqtt_initialized){
    initMQTTClient(LOCAL_ENV_MQTT_BROKER_HOST, LOCAL_ENV_MQTT_BROKER_PORT, AVAILABILITY_TOPIC.c_str());          
    mqtt_initialized = true;
    mqttclient.disconnect();  // necessary after init?       
    delay(100); // yield to allow for mqtt client activity post initialization    
  }
  
/* Runs until network and broker connectivity established and all subscriptions successful
   If network connectivity is lost and re-established, re-initialize the MQTT client
   If subscriptions fail, then will disconnect from broker, reconnect and try again. */    
  uint8_t failed_attempts = 0;
  do{
    if(connectMQTTBroker(DEVICE_ID, LOCAL_ENV_MQTT_USERNAME, LOCAL_ENV_MQTT_PASSWORD)){
      if(onMQTTConnect()){
        break;        
      }
    }    
    failed_attempts++;
    if(failed_attempts >= 10){      
      // reset!  -- alternately, just disconnect from wifi?
      ESP.restart(); 
    }
    else{
      Sprintln(F("ERROR: MQTT problem! Failed to connect to broker; retrying..."));     
      delay(MQTT_ATTEMPT_COOLDOWN);  // yield to allow for network and mqtt client activity between broker connection attempts              
    }
  } while(true); 

  pixels.clear();
  Sprintln("EXIT >>> assertMQTTConnectivity()");
}

void onNetworkConnect(){
  
  Sprintln("ENTER >>> onNetworkConnect()");
  Sprintln("Connected to network!");
  //print_heap();
  //printNetworkDetails();

  if(!time_is_set){
    setClock();
    time_is_set = true;
  }

  // ******************************************
  // connect to MQTT
  
  pixels.setPixelColor(0, pixels.Color(128, 0, 128)); // medium purple
  pixels.show();   // Send the updated pixel colors to the hardware.

  // Will execute (with cooldown between attempts) until connected to MQTT broker successfully, and successfully subscribed.
  assertMQTTConnectivity();  // initMQTTClient(), connectMQTTBroker(), onMQTTConnect()

  pixels.clear();  
  pixels.show(); 
  Sprintln("EXIT >>> onNetworkConnect()");
}

/*
  Will block until connected to wifi, waiting WIFI_ATTEMPT_COOLDOWN between attempts. Will reset device after too many failed attempts!
  Will block (waiting MQTT_ATTEMPT_COOLDOWN between attempts) until connected to MQTT broker successfully, and fully subscribed. 
*/
void assertConnectivity(){  
  // may reset device if failed to repeatedly connect to network
  if(assertNetworkConnectivity()){ // connectWifi() 
    // only if NEW connection established
    onNetworkConnect(); // setClock(), assertMQTTConnectivity() --> [initMQTTClient(), connectMQTTBroker(), onMQTTConnect()]
  }
  else{ // existing wifi network connection still working, check MQTT broker connection    
    if(!mqttclient.connected()){
      assertMQTTConnectivity();
    }
  }
  // at this point has network connection and broker connection, and is subscribed to topics of interest
}

void restart() {
  delay(10000); // 10s
  ESP.restart();
}

void initBME280(){
  // INITIALIZE BME280
  unsigned status = bme.begin();  
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  
  if (!status) {
      #ifndef DISABLE_SERIAL_OUTPUT
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      #endif

      indicateSensorProblem(0);
      displayBroken("Failed to initialize BME280!");
      restart();
  }  
}


void initPlayer(){
  // ******************************************
  // INITIALIZE VS1053 (music player)
  if (!musicPlayer.begin()) {
     Sprintln(F("Couldn't find VS1053, do you have the right pins defined?"));
     pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red
     pixels.show();
     displayBroken("Failed to initialize VS1053!");
     restart(); // there is a delay before reset
  }
  Sprintln(F("VS1053 found"));

  /*
    Enabling the use of interrupts is successful, BUT as soon as audio is played, a device reset occurs! (with both play full and start playing)
  */
  // If DREQ is on an interrupt pin we can do background audio playing (all ESP32 GPIO pins are interrupt-capable pins)
  //if(musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT)){
  // if(false){
  //   Sprintln(F("Interrupt use enabled"));
  // }    
  // else{
  //   Sprintln(F("Failed to enable interrupt use!"));
  // }

  //Sprintln(F("Activating test tone...")); 
  //musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
  
  if(!SD.begin(CARDCS)) {
    Sprintln(F("SD failed, or not present"));
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red
    pixels.show();
    displayBroken("Failed to access SDcard!");
    restart(); // there is a delay before reset
  }
  else{
    #ifndef DISABLE_SERIAL_OUTPUT
    Serial.println("SD found:");  
    printDirectory(SD.open("/"), 0);  
    #endif
  }
}


void initDisplay(){
  if(!display.begin(SSD1327_I2C_ADDRESS)){
    Sprintln(F("Unable to initialize display!"));
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // red
    pixels.show();    
    restart();
  }
  else{
    display.clearDisplay();
    display.display();
    Sprintln(F("Display initialized"));
  }
}


void setup() {

  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  #ifndef DISABLE_SERIAL_OUTPUT
  Serial.begin(115200);
  while (!Serial)
    delay(10); 

  Serial.println("ENTER >>> setup()");
  Serial.println(DEVICE_NAME);
  Serial.println(F("************************************"));
  
  Serial.print("CPU0 reset reason: ");
  //print_reset_reason(rtc_get_reset_reason(0));
  print_reset_reason_description(rtc_get_reset_reason(0));

  print_ESP32_info();
  #endif

  // ******************************************
  // INITIALIZE NeoPixel
  pixels.begin();  
  pixels.clear();
  
  initBME280();

  initPlayer();

  initDisplay();

  //displayMessage(2,"ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-+_=?[].");
  //displayMessage(1,"ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZanbdefghijklmnopqrstuvwxyz0123456789");

  // ******************************************
  // This metadata is assembled into HA-compatible discovery topics and payloads
  discovery_metadata_list = getAllDiscoveryMessagesMetadata(); 
  discovery_config_metadata_list = getAllDiscoveryConfigMessagesMetadata(); // must be defined before first assertConnectivity()
  discovery_measured_diagnostic_metadata_list = getAllDiscoveryMeasuredDiagnosticMessagesMetadata();
  discovery_fact_diagnostic_metadata_list = getAllDiscoveryFactDiagnosticMessagesMetadata();

  Sprintln("EXIT >>> setup()");
}

unsigned long lastMillis = 0;
bool lastPlayingState = false;
void loop()
{
  if(musicPlayer.playingMusic) {
    lastPlayingState = true;
    pixels.setPixelColor(0, pixels.Color(128, 128, 0)); // gold
    pixels.show();    
    musicPlayer.feedBuffer();
    //Sprint(">");
    pixels.clear();
    pixels.show();    
  }
  else{
    if(lastPlayingState){ // just finished playing

      // update the state to indicate that the device is no longer playing a tone.
      // when a deactivateSiren() call is made, it will already publish an updated state; but no harm in a duplicated OFF state update.
      std::string payload = "{\"state\": \""+OFF_VALUE+"\"}";
      publish(STATE_TOPIC.c_str(), payload.c_str());

      lastPlayingState = false;
    }
  }
  
  assertConnectivity(); // Blocks until network and MQTT broker connectivity established and all subscriptions successful

  mqttclient.loop(); // potential call to messageReceived()
  processMessages(); // deal with any pending_ops added by messageReceived() handler
    
  if (millis() - lastMillis > refresh_rate) {   
    lastMillis = millis();
      
    pixels.setPixelColor(0, pixels.Color(0, 128, 0)); // green
    pixels.show();

    publish();

    //std::string diagnostic_message = "RSSI: "+to_string(getRSSI())+"\nIP: "+getIP()+"\nMAC:"+getMAC()+"\nBoot:\n"+lastboot;
    //displayMessage("DIAGNOSTIC", diagnostic_message.c_str());
      
    pixels.clear();
    pixels.show();        
  }    
  yield();
}