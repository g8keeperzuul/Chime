# How to Use the MQTT for Home Assistant Library #
## Publishing Sensor MQTT Discovery Messages ##

## 0. Include the library ##
```
#include "mqtt-ha-helper.h"
```

## 1. Define some Details about the Device ##

Use these specific definitions. The value of DEVICE_ID must not have any spaces.
```
#define DEVICE_ID "featherm0"
#define DEVICE_NAME "AirMonitor"
#define DEVICE_MANUFACTURER "Adafruit"
#define DEVICE_MODEL "Feather M0"
#define DEVICE_VERSION "20221129.1400"
```

## 2. Define the Topics ##

All sensor updates are published in a single complex json payload to a single topic, so these same topics are used by all sensors or diagnostics.
The following **build*Topic()** methods are already supplied.
```
// Last will and testament topic
const std::string AVAILABILITY_TOPIC = buildAvailabilityTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/availability

const std::string STATE_TOPIC = buildStateTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/state
```

## 3. Define the Sensor(s) ##

Define a global variable to keep track of it all:
```
std::vector<discovery_metadata> discovery_metadata_list; 
```

Add one **discovery_metadata** instance for each sensor you want to announce to Home Assistant and return the collection via:
```
std::vector<discovery_metadata> getAllDiscoveryMessagesMetadata()
```

Attribute | Decription
--|--
device_class | Select from these [Home](https://www.home-assistant.io/integrations/sensor/#device-class) [Assistant](https://developers.home-assistant.io/docs/core/entity/sensor?_highlight=device&_highlight=class#available-device-classes) references.
has_sub_attr | Default is false, but set to "true" if you will be providing some sub-attributes in the payload.
icon | From [MaterialDesign](https://materialdesignicons.com/) reference
unit | Depends on device_class

Example:
```
  co2.device_class = "carbon_dioxide";
  co2.has_sub_attr = true; 
  co2.icon = "mdi:molecule-co2";
  co2.unit = "ppm"; 
```

The payload is expected to be modelled like this:
```
{
    "<device_class>": value,
    "<device_class>_details:{ <collection of any key-value pairs> }
}
```
The *_details attribute is only necessary if has_sub_attr = true

Example of a CO2 sensor that has it's own temperature and humidity sensors for calibration purposes:
```
{
    "carbon_dioxide": 1234.56,
    "carbon_dioxide_details": { "temperature": 23.4, "humidity": 45.0 }
}
```
Capturing details for controls, measurable diagnostics and [static diagnostics](Discovery_DiagnosticFact.md) is very similar.

## 4. Provide All Necessary Details to Build a Discovery Message ##

Implement **getDiscoveryMessage** that takes the discovery_metadata you just created plus some additional details and returns a **discovery_config** structure. All these methods are already defined and you already defined the device details and topics, so *all you need to do is cut-and-paste this into your code*.
```
discovery_config getDiscoveryMessage(discovery_metadata disc_meta){
    return getDiscoveryMessage( disc_meta, 
                                DEVICE_ID, 
                                buildDevicePayload( DEVICE_NAME, 
                                                    getMAC(), 
                                                    DEVICE_MANUFACTURER, 
                                                    DEVICE_MODEL, 
                                                    DEVICE_VERSION), 
                                AVAILABILITY_TOPIC, 
                                STATE_TOPIC);
}
```
## 5. Perform MQTT Discovery for all sensors, controls and diagnostics ##

Execute all the MQTT Discovery in **setup()** (*cut-and-paste below*)
```
  discovery_metadata_list = getAllDiscoveryMessagesMetadata(); 
  discovery_config_metadata_list = getAllDiscoveryConfigMessagesMetadata();
  discovery_measured_diagnostic_metadata_list = getAllDiscoveryMeasuredDiagnosticMessagesMetadata();
  discovery_fact_diagnostic_metadata_list = getAllDiscoveryFactDiagnosticMessagesMetadata();
  
  // Must successfully publish all discovery messages before proceding
  int discovery_messages_pending_publication;
  do {
    discovery_messages_pending_publication = publishDiscoveryMessages(); // Create the discovery messages and publish for each topic. Update published flag upon successful publication.
  }
  while(discovery_messages_pending_publication != 0);
```
## 6. Return Sensor Values ##

As per step 3, format your JSON payload like:
The payload is expected to be modelled like this:
```
{
    "<device_class>": value,
    "<device_class>_details:{ <collection of any key-value pairs> }
}
```
The *_details attribute is only necessary if has_sub_attr = true

and publish it to the **STATE_TOPIC** using:
```
void publish(String &topic, String &payload)
```
