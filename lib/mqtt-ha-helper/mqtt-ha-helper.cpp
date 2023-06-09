#include "mqtt-ha-helper.h"

/*
  This library is actually independent from wifi-helper.
  For that reason it is not possible to implement a universal MQTT broker + network connectivity
  method in this library. 
*/

void initMQTTClient(const IPAddress broker, int port, const char *lwt_topic)
{
  Sprint(F("Initalize MQTT client for broker:"));
  Sprint(broker); // IPAddress
  Sprint(":");
  Sprint(port); // int
  Sprint("... ");

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  // https://github.com/256dpi/arduino-mqtt/blob/master/src/MQTTClient.h#L101
  mqttclient.begin(broker, port, wificlient);

  // https://github.com/256dpi/arduino-mqtt/blob/master/src/MQTTClient.cpp#L199
  mqttclient.onMessage(messageReceived);

  mqttclient.setWill(lwt_topic, "offline", RETAINED, QOS_1);

  Sprintln(F("Done"));
}

// Returns TRUE if connected to MQTT broker
bool connectMQTTBroker(const char *client_id, const char *username, const char *password)
{  
  if(!mqttclient.connected())
  {
    Sprint(F("\nAttempting to connect to MQTT broker... "));
    if(mqttclient.connect(client_id, username, password)){
        Sprintln(F("Connected!"));
        delay(100);
        return true;
    }
    else{
        Sprintln(F("Failed!"));
        return false;
    }
  }
  else {
    return true;
  }
}

// Returns TRUE if a NEW broker connection was established
// Runs until connection to MQTT broker is established
// bool assertMQTTConnectivity(const char *client_id, const char *username, const char *password){    
//     if(mqttclient.connected()){
//         return false;
//     }
//     else{
//         while(!connectMQTTBroker(client_id, username, password)){
//             delay(100);
//         }
//         return true;
//     }
// }


void publish(const String &topic, const String &payload){
    Sprintln("\nPublishing message: " + topic + " : " + payload);
    //const char* payload_ch = payload.c_str();
    mqttclient.publish(topic, payload, NOT_RETAINED, QOS_0);
}

void publishOnline(const char* availability_topic){        
    Sprint("\nPublishing message: "); Sprint(availability_topic); Sprintln(" : online");
    mqttclient.publish(availability_topic, "online", RETAINED, QOS_1);     
}

/*
  Instead of actually publishing a MQTT message, insert an operation into the pending_ops queue as if
  a message had been processed by messageReceived().
  Control name is used instead of topic since the control_name is a required attribute of discovery_config_metadata.

  Returns true if control_name match found and pending operation has been queued, false otherwise.
*/
bool simulatePublish(const String &control_name, const String &payload){
  
  bool success = false;
  for (size_t i = 0; i < discovery_config_metadata_list.size(); i++){
    String test_control_name = String(discovery_config_metadata_list[i].control_name.c_str());
    if(test_control_name.equals(control_name)){
      // the message was received on a topic that we are subscribed to AND is a config/control topic
      pending_config_op op;
      op.config_meta = discovery_config_metadata_list[i];
      op.value = payload;

      pending_ops.push(op);
      success = true;

      break; // exit for-loop since topic found
    }
  }
  return success;
}

/**
 * @brief Subscribe to each of the provided topics (string)
 *
 * @param topicVector Vector of topic names as strings
 * @return true If ALL subscriptions were successful
 * @return false If any subscription failed
 */
bool subscribeTopics(std::vector<std::string> topicVector)
{
  bool completeSuccess = true;
  for (size_t i = 0; i < topicVector.size(); i++)
  {
    bool success = subscribeTopic(topicVector[i]);
    if (!success)
    {
      completeSuccess = false;
    }    
  }
  return completeSuccess;
}

bool subscribeTopic(std::string topic){  
  Sprint(F("Subscribe to topic ")); Sprint(topic.c_str()); Sprint("... ");
  bool success = mqttclient.subscribe(topic.c_str());
  if(success){
    Sprintln(F("OK"));    
  }
  else{
    Sprintln(F("FAILED!"));
  }
  return success;
}

/**
 * Pushes a pending operation onto the queue for later processing.
 * 
 * Main program must first() and pop() message requests from queue and process.
*/
void messageReceived(String &topic, String &payload) {
  Sprintln("\nIncoming message: " + topic + " : " + payload);

  // Note: Do not use the mqttclient in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `mqttclient.loop()`.

  // iterate thru config/contols until you have a setter topic that matches the inbound topic  
  for (size_t i = 0; i < discovery_config_metadata_list.size(); i++){
    String config_set_topic = String(discovery_config_metadata_list[i].set_topic.c_str());
    if(config_set_topic.equals(topic)){
      // the message was received on a topic that we are subscribed to AND is a config/control topic
      pending_config_op op;
      op.config_meta = discovery_config_metadata_list[i];
      op.value = payload;

      pending_ops.push(op);

      break; // exit for-loop since topic found
    }
  }
}


std::string buildAvailabilityTopic(const std::string device_type, const std::string device_id){  
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/availability --> homeassistant/sensor/esp8266thing/availability
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/availability";
  return topic;
}

std::string buildDiagnosticTopic(const std::string device_type, const std::string device_id){
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/diagnostics --> homeassistant/sensor/esp8266thing/diagnostics
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/diagnostics";
  return topic;
}

std::string buildStateTopic(const std::string device_type, const std::string device_id){
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/state --> homeassistant/sensor/esp8266thing/state
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/state";
  return topic;  
}

std::string buildDiscoveryTopic(const std::string device_type, const std::string device_id, const std::string sensor_id){
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/{sensor_id}/config --> homeassistant/sensor/esp8266thing/temp/config
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/"+sensor_id+"/config";
  return topic;
}

std::string buildSetterTopic(const std::string device_type, const std::string device_id, const std::string control_name){
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/{control_name}/set --> homeassistant/sensor/esp8266thing/refresh_rate/set
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/"+control_name+"/set";
  return topic;
}

std::string buildGetterTopic(const std::string device_type, const std::string device_id, const std::string control_name){
  // {HA_TOPIC_BASE}/{device_type}/{device_id}/{control_name}/get --> homeassistant/sensor/esp8266thing/refresh_rate/get
  std::string topic = std::string(HA_TOPIC_BASE)+"/"+device_type+"/"+device_id+"/"+control_name+"/get";
  return topic;
}

std::string buildDevicePayload(const std::string device_name, const std::string identifier, const std::string manufacturer, const std::string model, const std::string firmware_version){
  std::string payload = "{\"name\": \""+device_name+"\", \"identifiers\": \""+identifier+"\", \"mf\": \""+manufacturer+"\", \"mdl\": \""+model+"\", \"sw\": \""+firmware_version+"\"}"; 
  return payload;
}

std::string buildShortDevicePayload(const std::string device_name, const std::string identifier){
  std::string payload = "{\"name\": \""+device_name+"\", \"ids\": \""+identifier+"\"}"; 
  return payload;
}

/**
 * @brief Create a MQTT payload necessary for automatic discovery within Home Assistant.
 * 
 * @param device_class https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
 * @param device_id Unique identifier for this device
 * @param json_attr top-tier json attribute this sensor references
 * @param has_sub_attr flag that indicates json_attr has sub-attributes. Will define a key called "<json_attr>_details", so sensor details must use that.
 * @param icon https://materialdesignicons.com/
 * @param unit see supported units column under https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes
 * @param identifier Unique device identifer. Generally use wireless MAC address.
 * @param avail_topic Availability topic for this sensor (on a device with many sensors this will be shared)
 * @param state_topic State topic for this sensor (on a device with many sensors this will generally be shared)
 * @return std::string 
 */
std::string buildDiscoveryPayload(const std::string device_class, const std::string device_id, const std::string json_attr, bool has_sub_attr, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic){
  std::string payload = "{\
\"device_class\":\""+device_class+"\",\
\"unit_of_measurement\":\""+unit+"\",\
\"state_class\":\"measurement\",\
\"availability_topic\":\""+avail_topic+"\",\
\"unique_id\":\""+device_id+"_"+json_attr+"\",\
\"device\":"+device_payload+",\
\"name\":\""+device_id+" "+json_attr+"\",\
\"icon\":\""+icon+"\",\
\"state_topic\":\""+state_topic+"\",\
\"value_template\":\"{{ value_json."+json_attr+" }}\"";

  if(has_sub_attr){
    payload = payload + ",\
\"json_attributes_topic\": \""+state_topic+"\",\
\"json_attributes_template\": \"{{ value_json."+json_attr+"_details | tojson }}\"";
  }

  payload = payload + "}";

  return payload;
}

/**
 * @brief A special kind of discovery message for controls (ideally they support bidirectional getter and setter messages 
 * so that Home Assistant UI actually reflects the real control state rather than what it optimistically thinks it to be). 
 * The component type (such as 'number', 'switch') is defined in the config topic (not used here).
 * See https://www.home-assistant.io/docs/mqtt/discovery
 * 
 * @param device_id A unique device ID
 * @param config_attr A unique label used in the name and unique_id.
 * @param custom_settings A string with quoted key-value pairs separated by colons and commas just as JSON dictates 
 * (will be appended to config payload). The specifics depend on the device type that this control uses. Pass empty string if no additional settings.
 * See https://www.home-assistant.io/docs/mqtt/discovery
 * @param icon https://materialdesignicons.com/
 * @param unit Specific to the quantity that the control is setting (ie volume, minutes, etc)
 * @param device_payload Unique device identifer. Multiple attributes provided by buildDevicePayload()
 * @param avail_topic Availability topic for this control (on a device with many controls and/or sensors this will be shared)
 * @param state_topic The getter state topic for the state of this control. The topic payload should only be the value to be reflected by the HA UI.
 * @param command_topic The setter state topic to update the state of this control. The topic payload should only contain the value to set. 
 * @return std::string 
 */
std::string buildDiscoveryConfigPayload(const std::string device_id, const std::string config_attr, const std::string custom_settings, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic, const std::string command_topic){
  std::string payload = "{\
\"entity_category\": \"config\",\
\"unit_of_measurement\": \""+unit+"\",\
\"availability_topic\":\""+avail_topic+"\",\
\"unique_id\":\""+device_id+"_"+config_attr+"\",\
\"device\":"+device_payload+",\
\"name\":\""+device_id+" "+config_attr+"\",\
\"icon\":\""+icon+"\",\
\"state_topic\":\""+state_topic+"\",\
\"command_topic\": \""+command_topic+"\"";

  if(custom_settings.length() > 0){
    payload = payload + ", "+custom_settings;
  }
  payload = payload + "}";

  return payload;
}

std::string buildDiscoveryDiagnosticMeasurementPayload(const std::string state_class, const std::string device_class, const std::string device_id, const std::string diag_attr, const std::string icon, const std::string unit, const std::string device_payload, const std::string avail_topic, const std::string state_topic){

// If device_class or unit_of_measurement is not provided, do not include in payload (not even if value is set to None or empty string)
// std:string cannot be a null pointer, it will always have a value.

std::string payload = "{ ";
if(!device_class.empty()){
  payload = payload + "\"device_class\":\""+device_class+"\",";
}
if(!unit.empty()){
  payload = payload + "\"unit_of_measurement\": \""+unit+"\",";
}
payload = payload + "\
\"state_class\":\""+state_class+"\",\
\"entity_category\": \"diagnostic\",\
\"availability_topic\":\""+avail_topic+"\",\
\"unique_id\":\""+device_id+"_"+diag_attr+"\",\
\"device\":"+device_payload+",\
\"name\":\""+device_id+" "+diag_attr+"\",\
\"icon\":\""+icon+"\",\
\"state_topic\":\""+state_topic+"\",\
\"value_template\":\"{{ value_json."+diag_attr+" }}\" }";
  
  return payload;  
}

std::string buildDiscoveryDiagnosticFactPayload(const std::string device_id, const std::string diag_attr, const std::string icon, const std::string device_payload, const std::string avail_topic, const std::string state_topic){
  std::string payload = "{\
\"entity_category\": \"diagnostic\",\
\"availability_topic\":\""+avail_topic+"\",\
\"unique_id\":\""+device_id+"_"+diag_attr+"\",\
\"device\":"+device_payload+",\
\"name\":\""+device_id+" "+diag_attr+"\",\
\"icon\":\""+icon+"\",\
\"state_topic\":\""+state_topic+"\",\
\"value_template\":\"{{ value_json."+diag_attr+" }}\" }";
  
  return payload;  
}

/**
 * For generating topic and payload for sensor discovery messages
 * These messages are large (specifically sensor config), so use the shorter device payload; since the ID is the same as the longer 
 * format all the sensor config messages will effectively inherit the additional device details.
 * The topic names could be generated within this method, but to save memory they are created once by the main program and passed in.
 */
// build discovery message - step 4 of 4
discovery_config getDiscoveryMessage(discovery_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic){
    discovery_config disc;
    disc.topic = buildDiscoveryTopic(disc_meta.device_type, device_id, disc_meta.device_class /*sensor_id*/); 
    disc.payload = buildDiscoveryPayload(disc_meta.device_class, device_id, disc_meta.device_class /*json_attr*/, disc_meta.has_sub_attr, disc_meta.icon, disc_meta.unit, device_payload, avail_topic, state_topic);
    return disc;  
}

/** 
 * For generating topic and payload for control/config discovery messages
 * The topic names could be generated within this method, but to save memory they are created once by the main program and passed in.
 * Since these messages are shorter in nature, use the fully defined device payload
*/
// build discovery configuration/control message - step 4 of 4
discovery_config getDiscoveryConfigMessage(discovery_config_metadata disc_meta, const std::string device_id, std::string device_payload, const std::string avail_topic){
  discovery_config disc;
  const std::string sensor_id = disc_meta.control_name;

  std::string get_topic = disc_meta.get_topic;
  if(get_topic.empty()) // null check
    get_topic = buildGetterTopic(disc_meta.device_type, device_id, disc_meta.control_name);
  
  std::string set_topic = disc_meta.set_topic;
  if(set_topic.empty()) // null check
    set_topic = buildSetterTopic(disc_meta.device_type, device_id, disc_meta.control_name);  

  disc.topic = buildDiscoveryTopic(disc_meta.device_type, device_id, sensor_id);
  disc.payload = buildDiscoveryConfigPayload(device_id, disc_meta.control_name, disc_meta.custom_settings, disc_meta.icon, disc_meta.unit, device_payload, avail_topic, get_topic, set_topic);
  return disc;
}

discovery_config getDiscoveryMeasuredDiagnosticMessage(discovery_measured_diagnostic_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic){
    discovery_config disc;
    disc.topic = buildDiscoveryTopic(disc_meta.device_type, device_id, disc_meta.diag_attr /*sensor_id*/); 
    disc.payload = buildDiscoveryDiagnosticMeasurementPayload(disc_meta.state_class, disc_meta.device_class, device_id, disc_meta.diag_attr, disc_meta.icon, disc_meta.unit, device_payload, avail_topic, state_topic);
    return disc;  
}

discovery_config getDiscoveryFactDiagnosticMessage(discovery_fact_diagnostic_metadata disc_meta, std::string device_id, std::string device_payload, std::string avail_topic, std::string state_topic){
    discovery_config disc;
    disc.topic = buildDiscoveryTopic(disc_meta.device_type, device_id, disc_meta.diag_attr /*sensor_id*/); 
    disc.payload = buildDiscoveryDiagnosticFactPayload(device_id, disc_meta.diag_attr, disc_meta.icon, device_payload, avail_topic, state_topic);
    return disc;  
}

/**
 * @brief Publish the given payload for each topic. Update published flag upon successful publication.
 *
 * @return The number of unpublished messages left (due to failure)
 */
// build discovery and discovery config/control message - step 2 of 4
int publishDiscoveryMessages()
{
  int pending_discovery_count = discovery_metadata_list.size() + discovery_config_metadata_list.size() + discovery_measured_diagnostic_metadata_list.size() + discovery_fact_diagnostic_metadata_list.size();
  
  // Metadata is different for discovery_metadata and discovery_config_metadata but both can create a discovery_config with topic and payload
  for (size_t i = 0; i < discovery_metadata_list.size(); i++)
  {
    if (!discovery_metadata_list[i].published)
    {
      // generate topic and payload one at a time
      discovery_config disc = getDiscoveryMessage(discovery_metadata_list[i]); // build discovery message - step 3
      
      Sprint(F("\nPublishing discovery message to "));
      Sprint(disc.topic.c_str());
      if (!mqttclient.publish(disc.topic.c_str(), disc.payload.c_str(), RETAINED, QOS_1))
      { // should return true if successfully sent and since QoS=1 ACK should also be received
        Sprintln(F("... Failed"));
      }
      else
      {
        Sprintln(F("... OK"));
        discovery_metadata_list[i].published = true;
        pending_discovery_count--; // just published

        // TODO: delete all attributes except published flag to same RAM? (note: this is done after all messages have been published)    
      }
    }
    else{
        pending_discovery_count--; // previously published
    }
  }
  
  for (size_t i = 0; i < discovery_config_metadata_list.size(); i++)
  {
    if (!discovery_config_metadata_list[i].published)
    {
      // generate topic and payload one at a time
      discovery_config disc = getDiscoveryMessage(discovery_config_metadata_list[i]); // build discovery configuration/control message - step 3
      
      Sprint(F("\nPublishing configuration/control discovery message to "));
      Sprint(disc.topic.c_str());
      if (!mqttclient.publish(disc.topic.c_str(), disc.payload.c_str(), RETAINED, QOS_1))
      { // should return true if successfully sent and since QoS=1 ACK should also be received
        Sprintln(F("... Failed"));
      }
      else
      {
        Sprintln(F("... OK"));
        discovery_config_metadata_list[i].published = true;
        pending_discovery_count--; // just published 

        // TODO: delete all attributes except published flag to same RAM? (note: this is done after all messages have been published)
      }
    }
    else{
        pending_discovery_count--; // previously published
    }
  }  
  
  for (size_t i = 0; i < discovery_measured_diagnostic_metadata_list.size(); i++)
  {
    if (!discovery_measured_diagnostic_metadata_list[i].published)
    {
      // generate topic and payload one at a time
      discovery_config disc = getDiscoveryMessage(discovery_measured_diagnostic_metadata_list[i]); // build discovery measured diagnostic message - step 3
      
      Sprint(F("\nPublishing measured diagnostic discovery message to "));
      Sprint(disc.topic.c_str());
      if (!mqttclient.publish(disc.topic.c_str(), disc.payload.c_str(), RETAINED, QOS_1))
      { // should return true if successfully sent and since QoS=1 ACK should also be received
        Sprintln(F("... Failed"));
      }
      else
      {
        Sprintln(F("... OK"));
        discovery_measured_diagnostic_metadata_list[i].published = true;
        pending_discovery_count--; // just published 

        // TODO: delete all attributes except published flag to same RAM? (note: this is done after all messages have been published)
      }
    }
    else{
        pending_discovery_count--; // previously published
    }
  } 
  
  for (size_t i = 0; i < discovery_fact_diagnostic_metadata_list.size(); i++)
  {
    if (!discovery_fact_diagnostic_metadata_list[i].published)
    {
      // generate topic and payload one at a time
      discovery_config disc = getDiscoveryMessage(discovery_fact_diagnostic_metadata_list[i]); // build discovery diagnostic fact message - step 3
      
      Sprint(F("\nPublishing diagnostic fact discovery message to "));
      Sprint(disc.topic.c_str());
      if (!mqttclient.publish(disc.topic.c_str(), disc.payload.c_str(), RETAINED, QOS_1))
      { // should return true if successfully sent and since QoS=1 ACK should also be received
        Sprintln(F("... Failed"));
      }
      else
      {
        Sprintln(F("... OK"));
        discovery_fact_diagnostic_metadata_list[i].published = true;
        pending_discovery_count--; // just published 

        // TODO: delete all attributes except published flag to same RAM? (note: this is done after all messages have been published)
      }
    }
    else{
        pending_discovery_count--; // previously published
    }
  }
  
  return pending_discovery_count;
}

/**
 * Populate list of topics to subscribe to
 * This is derived from discovery_config_metadata_list. If getter and setter topics are not explicitly defined for each config/control,
 * then defaults are generated. 
 * A list of setter topics is returned. 
 */
std::vector<std::string> getAllSubscriptionTopics(std::string device_id){    
  std::vector<std::string> topics = { }; 

  // getter and setter topics are only defined for config/control messages
  for (size_t i = 0; i < discovery_config_metadata_list.size(); i++){
    // if a custom setter topic is not provided, create one with the default naming convention
    if(discovery_config_metadata_list[i].set_topic.empty()){
      discovery_config_metadata_list[i].set_topic = buildSetterTopic(discovery_config_metadata_list[i].device_type, device_id, discovery_config_metadata_list[i].control_name);
//    Sprint("Initializing setter topic for "); Sprint(discovery_config_metadata_list[i].control_name.c_str()); Sprint(" : "); Sprintln(discovery_config_metadata_list[i].set_topic.c_str());
    }
    // if a custom getter topic is not provided, create one with the default naming convention
    if(discovery_config_metadata_list[i].get_topic.empty()){
      discovery_config_metadata_list[i].get_topic = buildGetterTopic(discovery_config_metadata_list[i].device_type, device_id, discovery_config_metadata_list[i].control_name);      
//    Sprint("Initializing getter topic for "); Sprint(discovery_config_metadata_list[i].control_name.c_str()); Sprint(" : "); Sprintln(discovery_config_metadata_list[i].get_topic.c_str());
    }    
//  Sprint("Adding subscription topic: "); Sprintln(discovery_config_metadata_list[i].set_topic.c_str());
    topics.push_back(discovery_config_metadata_list[i].set_topic);
  }  
  return topics;
}

void purgeDiscoveryMetadata(){
  for (size_t i = 0; i < discovery_metadata_list.size(); i++)
  {
    if (discovery_metadata_list[i].published)
    {
      discovery_metadata_list[i].device_type.clear();
      discovery_metadata_list[i].device_class.clear();
      discovery_metadata_list[i].icon.clear();
      discovery_metadata_list[i].unit.clear();       
    }
  }
  for (size_t i = 0; i < discovery_config_metadata_list.size(); i++)
  {
    if (discovery_config_metadata_list[i].published)
    {
      discovery_config_metadata_list[i].control_name.clear();
      discovery_config_metadata_list[i].custom_settings.clear();
      discovery_config_metadata_list[i].device_type.clear();
      discovery_config_metadata_list[i].icon.clear();
      discovery_config_metadata_list[i].unit.clear();
    }
  }  
  for (size_t i = 0; i < discovery_measured_diagnostic_metadata_list.size(); i++)
  {
    if (discovery_measured_diagnostic_metadata_list[i].published)
    {
      discovery_measured_diagnostic_metadata_list[i].device_type.clear();
      discovery_measured_diagnostic_metadata_list[i].device_class.clear();
      discovery_measured_diagnostic_metadata_list[i].state_class.clear();
      discovery_measured_diagnostic_metadata_list[i].diag_attr.clear();
      discovery_measured_diagnostic_metadata_list[i].icon.clear();
      discovery_measured_diagnostic_metadata_list[i].unit.clear();
    }
  }   
  for (size_t i = 0; i < discovery_fact_diagnostic_metadata_list.size(); i++)
  {
    if (discovery_fact_diagnostic_metadata_list[i].published)
    {
      discovery_fact_diagnostic_metadata_list[i].device_type.clear();            
      discovery_fact_diagnostic_metadata_list[i].diag_attr.clear();
      discovery_fact_diagnostic_metadata_list[i].icon.clear();      
    }
  }  
}
