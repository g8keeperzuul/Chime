#ifndef MQTT_HA_HELPER_H
#define MQTT_HA_HELPER_H

#include <WiFi.h> // for WiFiClient
#include <MQTT.h>
#include <vector>
#include <queue>
#include <string>
#include "log.h"

// Not used by this library, but meant to be used by the users of this library between calls to connectMQTTBroker()
#define MQTT_ATTEMPT_COOLDOWN 10000 // milliseconds between MQTT broker connection attempts

// *** MQTT Related Constants ***
#define RETAINED true
#define NOT_RETAINED false
// https://github.com/256dpi/arduino-mqtt/blob/master/src/lwmqtt/lwmqtt.h#L66
#define QOS_0 0
#define QOS_1 1

// Problem return codes to be used with indicateMQTTProblem()
#define MQTT_CONN_ERR 1
#define MQTT_SUB_ERR 2

#define HA_TOPIC_BASE "homeassistant"

/*
const std::string HA_TOPIC_BASE = std::string("homeassistant");

// *** MQTT Related Constants ***
const bool RETAINED = true;
const bool NOT_RETAINED = false;
// https://github.com/256dpi/arduino-mqtt/blob/master/src/lwmqtt/lwmqtt.h#L66
const int QOS_0 = 0;
const int QOS_1 = 1;

// Problem return codes to be used with indicateMQTTProblem()
const byte MQTT_CONN_ERR = 1;
const byte MQTT_SUB_ERR = 2;
*/

// *********************************************************************************************************************
// *** Data Types ***
struct discovery_metadata{
  std::string device_type = "sensor"; // Entity such as number | switch | light | sensor ...  https://developers.home-assistant.io/docs/core/entity  
  std::string device_class;       // https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
                                  // On the state output json, name the attribute for the sensor the same as the device_class (i.e. PM2.5 should be => pm25, temp should be => temperature)
  bool has_sub_attr = false;      // if true, adds json_attributes_topic (same as state topic) and json_attributes_template which will parse out the "<attrib>_details" field and apply all contents found
                                  // This means your payload should have a "<attrib>_details: {...}" where all sub-attributes will be associated with "<attrib>" in Home Assistant.
  std::string icon;               // https://materialdesignicons.com/
  std::string unit;               // see supported units for device_class (can make up your own unit as well) https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
  bool published = false;         // publication success flag; set automatically
};

struct discovery_config_metadata{
  std::string device_type;        // Entity such as number | switch | light | sensor ...  https://developers.home-assistant.io/docs/core/entity
  std::string control_name;       // internal label for control (no spaces)
  std::string custom_settings;    // JSON snippet with escaped quotes - contents depend on device_type - ie. "\"min\": 1, \"max\": 10000"
  std::string icon;               // https://materialdesignicons.com/
  std::string unit;               // ppm, ticks, meters, C, F, (anything)
  std::string set_topic;          // Topic used to set configuration; set automatically if not provided
  std::string get_topic;          // Topic used to get configuration; set automatically if not provided
  bool published = false;         // publication success flag; set automatically
};

// Diagnostic can either be a fact (like IP address) or a measurable value (like RSSI)

struct discovery_measured_diagnostic_metadata{
  std::string device_type;        // Entity such as number | switch | light | sensor ...  https://developers.home-assistant.io/docs/core/entity
  std::string device_class;       // battery | date | duration | timestamp | ... In some cases may be "None" - https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes    
  std::string state_class = "measurement"; // measurement | total | total_increasing
  std::string diag_attr;          // Name of json attribute within diagnostic message payload
  std::string icon;               // https://materialdesignicons.com/
  std::string unit;               // ppm, ticks, meters, C, F, (anything)
  bool published = false;         // publication success flag; set automatically
};

struct discovery_fact_diagnostic_metadata{  
  std::string device_type;        // Entity such as number | switch | light | sensor ...  https://developers.home-assistant.io/docs/core/entity
  std::string diag_attr;          // Name of json attribute within diagnostic message payload
  std::string icon;               // https://materialdesignicons.com/  
  bool published = false;         // publication success flag; set automatically
};

struct discovery_config{          // Home Assistant MQTT Discovery https://www.home-assistant.io/docs/mqtt/discovery/
  std::string topic;              // discovery topic in the form <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
  std::string payload;            // discovery details
};

// messageReceived() pushes instance of pending_config_op to global pending_ops queue
struct pending_config_op{
  discovery_config_metadata config_meta; // details of the request, including setter topic message received on
  String value;                   // the value sent to the setter topic
};

// *********************************************************************************************************************
// *** Must Declare ***
extern MQTTClient mqttclient;
extern WiFiClient wificlient;
extern std::vector<discovery_metadata> discovery_metadata_list;                                         // list of data used to construct discovery_config for sensor discovery
extern std::vector<discovery_config_metadata> discovery_config_metadata_list;                           // list of data used to construct discovery_config for config/control discovery
extern std::vector<discovery_measured_diagnostic_metadata> discovery_measured_diagnostic_metadata_list; // list of data used to construct discovery_config for measured diagnostics discovery (like RSSI) 
extern std::vector<discovery_fact_diagnostic_metadata> discovery_fact_diagnostic_metadata_list;         // list of data used to construct discovery_config for diagnostic facts discovery (like IP address)
extern std::queue<pending_config_op> pending_ops;                                                      // queue used to record incoming requests to setter topics for later processing

// *** Must Implement ***

// Main program must implement
std::vector<discovery_metadata> getAllDiscoveryMessagesMetadata();                                      // define specific sensor discovery facts that are to be discoverable (build discovery message - step 1 of 4)
discovery_config getDiscoveryMessage(discovery_metadata disc_meta);                                     // build discovery message - step 3 of 4

std::vector<discovery_config_metadata> getAllDiscoveryConfigMessagesMetadata();                         // define specific device configuration/control discovery facts that are to be discoverable
discovery_config getDiscoveryMessage(discovery_config_metadata disc_meta);                              // build discovery config/control message - step 3 of 4; convert config/control discovery facts to discovery topic and payload

std::vector<discovery_measured_diagnostic_metadata> getAllDiscoveryMeasuredDiagnosticMessagesMetadata();// define specific device measurable diagnostics that are to be discoverable
discovery_config getDiscoveryMessage(discovery_measured_diagnostic_metadata disc_meta);                 // build discovery measurable diagnostic message - step 3 of 4; convert measurable diagnostic discovery facts to discovery topic and payload

std::vector<discovery_fact_diagnostic_metadata> getAllDiscoveryFactDiagnosticMessagesMetadata();        // define specific device measurable diagnostics that are to be discoverable
discovery_config getDiscoveryMessage(discovery_fact_diagnostic_metadata disc_meta);                     // build discovery measurable diagnostic message - step 3 of 4; convert measurable diagnostic discovery facts to discovery topic and payload

void messageReceived(String &topic, String &payload);                                                   // handler for each subscribed topic


// Provided in library
// Connectivity and basic operations
void initMQTTClient(const IPAddress broker, int port, const char *lwt_topic); 
bool connectMQTTBroker(const char *client_id, const char *username, const char *password);
void indicateMQTTProblem(byte return_code);
void publish(const String &topic, const String &payload);
void publishOnline(const char* availability_topic);
bool simulatePublish(const String &control_name, const String &payload);
bool subscribeTopics(std::vector<std::string> topicVector);
bool subscribeTopic(std::string topic);
int publishDiscoveryMessages();                                                                         // build discovery message - step 2 of 4
std::vector<std::string> getAllSubscriptionTopics(std::string device_id);                               // return list of topics to be subscribed to 
void purgeDiscoveryMetadata();

// Topic builders
std::string buildAvailabilityTopic(const std::string device_type, const std::string device_id);
std::string buildStateTopic(const std::string device_type, const std::string device_id);
std::string buildDiagnosticTopic(const std::string device_type, const std::string device_id);
std::string buildDiscoveryTopic(const std::string device_type, const std::string device_id, const std::string sensor_id);
std::string buildSetterTopic(const std::string device_type, const std::string device_id, const std::string control_name);
std::string buildGetterTopic(const std::string device_type, const std::string device_id, const std::string control_name);

// Payload builders
std::string buildShortDevicePayload(const std::string device_name, const std::string identifier);   // build discovery message - part of step 3
std::string buildDevicePayload(const std::string device_name, const std::string identifier, const std::string manufacturer, const std::string model, const std::string version);  // build discovery message - part of step 4
std::string buildDiscoveryPayload(const std::string device_class, const std::string device_id, const std::string json_attr, bool has_sub_attr, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic);  // build discovery message - part of step 4
std::string buildDiscoveryConfigPayload(const std::string device_id, const std::string config_attr, const std::string custom_settings, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic, const std::string command_topic); // build discovery configuration/control message 
std::string buildDiscoveryDiagnosticMeasurementPayload(const std::string state_class, const std::string device_class, const std::string device_id, const std::string diag_attr, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic);
std::string buildDiscoveryDiagnosticFactPayload(const std::string device_id, const std::string diag_attr, const std::string icon, const std::string device_payload, const std::string avail_topic, const std::string state_topic);

// For generating topic and payload for sensor discovery messages                   
discovery_config getDiscoveryMessage(discovery_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic); // build discovery message - step 4 of 4
discovery_config getDiscoveryConfigMessage(discovery_config_metadata disc_meta, const std::string device_id, std::string device_payload, const std::string avail_topic); // build discovery config/control message - step 4 of 4
discovery_config getDiscoveryMeasuredDiagnosticMessage(discovery_measured_diagnostic_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic);
discovery_config getDiscoveryFactDiagnosticMessage(discovery_fact_diagnostic_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic);

#endif