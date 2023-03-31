# How to Use the MQTT for Home Assistant Library #
## Publishing Measurable Diagnostic MQTT Discovery Messages ##

Steps 0,1 & 5 are unchanged

## 2C. Define the Diagnostic Topic ##
(Same as Step 2B for Diagnostic Facts)

The availability topic already defined is used as is.
The **buildDiagnosticTopic()** method are are included in the library.
```
const std::string DIAGNOSTIC_TOPIC = buildDiagnosticTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/diagnostics
```

## 3C. Define the Measurable Diagnostics ##

Define a global variable to keep track of it all:
```
std::vector<discovery_measured_diagnostic_metadata> discovery_measured_diagnostic_metadata_list;
```

Add one **discovery_measured_diagnostic_metadata** instance for each measurable diagnostic you want to announce to Home Assistant and return the collection via:
```
std::vector<discovery_measured_diagnostic_metadata> getAllDiscoveryMeasuredDiagnosticMessagesMetadata()
```

Examples of measurable diagnostics, may be the device's network connection strength (RSSI), total operating hours, etc. Generally any chartable diagnostic. 

Attribute | Decription
--|--
device_type | Entity such as number, switch, light, sensor...  [See Home Assistant docs](https://developers.home-assistant.io/docs/core/entity)
device_class | battery, date, duration, times... or empty. [See Home Assistant docs](https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes)
state_class | Default is "measurement", can be "total" or "total_increasing"
diag_attr | Name of json attribute within diagnostic message payload
icon | From [MaterialDesign](https://materialdesignicons.com/) reference
unit | Leave empty if unitless (like RSSI)


Example - the device's network connection strength:
```
  rssi.device_type = "sensor";
  rssi.device_class = "";
  //rssi.state_class = "measurement";  Accept default
  rssi.diag_attr = "wifi_rssi";
  rssi.icon = "mdi:wifi-strength-2"; 
  rssi.unit = ""; // RSSI is unitless  
```

## 4C. Provide All Necessary Details to Build a Measurable Diagnostic Discovery Message ##

Implement **getDiscoveryMessage** that takes the **discovery_measured_diagnostic_metadata** you just created plus some additional details and returns a **discovery_config** structure. All these methods are already defined and you already defined the device details and topics, so *all you need to do is cut-and-paste this into your code*.
```
discovery_config getDiscoveryMessage(discovery_measured_diagnostic_metadata disc_meta){
    return getDiscoveryFactDiagnosticMessage( 
                                disc_meta, 
                                DEVICE_ID, 
                                buildShortDevicePayload( DEVICE_NAME, getMAC() ), 
                                AVAILABILITY_TOPIC, 
                                DIAGNOSTIC_TOPIC);
}
```

## 6C. Return Diagnostic Values ##

As per Step 3C, format your JSON payload like:
```
{
    "<diag_attr>": value,    
}
```

Example diagnostic payload:
```
{ "wifi_rssi": -42 }
```

and publish it to the **DIAGNOSTIC_TOPIC** using:
```
void publish(String &topic, String &payload)
```

