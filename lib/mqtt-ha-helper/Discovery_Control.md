# How to Use the MQTT for Home Assistant Library #
## Publishing Config/Control MQTT Discovery Messages ##

## 2D. Define the Topics ##

All sensor updates are published in a single complex json payload to a single topic, so these same topics are used by all sensors or diagnostics.
The following **build*Topic()** methods are already supplied.
```
// Last will and testament topic
const std::string AVAILABILITY_TOPIC = buildAvailabilityTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/availability

const std::string STATE_TOPIC = buildStateTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/state
```

Controls/Configuration Discovery is a special case that requires a dedicated topic for getting and setting the control.

```
homeassistant/{device_type}/{device_id}/{control_name}/set
homeassistant/{device_type}/{device_id}/{control_name}/get
```

The **device_type** and **control_name** are defined in the next step (3D). The **device_id** was defined in step 1.

## 3D. Define the Control(s) ##

Define a global variable to keep track of it all:
```
std::vector<discovery_config_metadata> discovery_config_metadata_list; 
```

Add one **discovery_config_metadata** instance for each sensor you want to announce to Home Assistant and return the collection via:
```
std::vector<discovery_config_metadata> getAllDiscoveryConfigMessagesMetadata()
```

Attribute | Decription
--|--
device_type | Entity such as number, switch, light, sensor...  [See Home Assistant docs](https://developers.home-assistant.io/docs/core/entity)
control_name | Internal label for control (no spaces)
custom_settings | JSON snippet with escaped quotes - contents depend on device_type - ie. "\"min\": 1, \"max\": 10000"
icon | From [MaterialDesign](https://materialdesignicons.com/) reference
unit | Any unit
set_topic | (optional) Name of topic used to set the value of this control
get_topic | (optional) Name of topic used to reflect back the actual state of this control to all subscribers

Example:
```
  tempoffset.device_type = "number";
  tempoffset.control_name = "temperature_offset";
  tempoffset.custom_settings = "\"min\": 0, \"max\": 10000, \"step\": 10, \"initial\": 0";
  tempoffset.icon = "mdi:home-thermometer";
  tempoffset.unit = "hundredths °C";
  //tempoffset.set_topic = if not set, it will be generated
  //tempoffset.get_topic = if not set, it will be generated  
```

For each config/control, a getter and setter can be set. If not provided, then default topic names are used
(see **buildGetterTopic()** and **buildSetterTopic()**)

## 4D. Provide All Necessary Details to Build a Config/Control Discovery Message ##

Implement **getDiscoveryMessage** that takes the **discovery_config_metadata** you just created plus some additional details and returns a **discovery_config** structure. All these methods are already defined and you already defined the device details and topics, so *all you need to do is cut-and-paste this into your code*.
```
discovery_config getDiscoveryMessage(discovery_config_metadata disc_meta){
    return getDiscoveryConfigMessage( disc_meta, 
                                      DEVICE_ID, 
                                      buildDevicePayload( DEVICE_NAME, 
                                                          getMAC(), 
                                                          DEVICE_MANUFACTURER, 
                                                          DEVICE_MODEL, 
                                                          DEVICE_VERSION), 
                                      AVAILABILITY_TOPIC);                                 
}
```
## 6D. Return Config/Control Values ##

(optional) You could return config/control values as part of the diagnostic message. Any updates to these values by the setter topic will be immediately reflected by the getter topic, but not in the diagnostic payload. Diagnostic values are only updated on a fixed periodic schedule. 

## 7 (NEW). Implement Config/Control Update Handler ##

This step is unique to config/control elements since an action needs to be performed when receiving an MQTT message. This is typically setting a device configuration value or toggling a capability. 

The device will subscribe to all config/control setter topics so that updates can be received via MQTT.
Once the config/control has been updated, the device will publish the new value to the getter topic so that Home Assistant can accurately reflect the device state.

Subscription is handled automatically by the library. The list of all setter topics and subscription to each of them is automatically handled as part of **assertConnectivity()**.

When a message is received on a subscribed-to topic, the generic subscription handler is invoked automatically:
```
void messageReceived(String &topic, String &payload)
```
which will push a pending operation onto a queue for later processing.

_The main program must first() and pop() message requests from **pending_ops** queue and process._
This is where the actual request is handled.
```
while(!pending_ops.empty()){
    pending_config_op op = pending_ops.front();
    Serial.print(F("Processing pending message : ")); Serial.println(op.config_meta.control_name.c_str());

    if(op.config_meta.control_name.compare("temperature_offset") == 0){ 
      // get the provided value
      int some_value = op.value.toInt(); // datatype will depend on config/control
      ... handle message ... 
      // publish updated value so that Home Assistant reflects device actual state
      publish(op.config_meta.get_topic.c_str(), String(some_value));
    }
}
```

