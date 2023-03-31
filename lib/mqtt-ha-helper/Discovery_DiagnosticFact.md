# How to Use the MQTT for Home Assistant Library #
## Publishing Diagnostic Fact MQTT Discovery Messages ##

Steps 0,1 & 5 are unchanged

## 2B. Define the Diagnostic Topic ##
The availability topic already defined is used as is.
The **buildDiagnosticTopic()** method are are included in the library.
```
const std::string DIAGNOSTIC_TOPIC = buildDiagnosticTopic("sensor", std::string(DEVICE_ID)); // homeassistant/sensor/featherm0/diagnostics
```

## 3B. Define the Static Diagnostics ##

Define a global variable to keep track of it all:
```
std::vector<discovery_fact_diagnostic_metadata> discovery_fact_diagnostic_metadata_list;
```

Add one **discovery_fact_diagnostic_metadata** instance for each diagnostic fact you want to announce to Home Assistant and return the collection via:
```
std::vector<discovery_fact_diagnostic_metadata> getAllDiscoveryFactDiagnosticMessagesMetadata()
```

Examples of diagnostic facts (static facts), may be the device's IP address, MAC, current firmware version, etc. Generally any non-chartable diagnostic. 

Attribute | Decription
--|--
device_type | Entity such as number, switch, light, sensor...  [See Home Assistant docs](https://developers.home-assistant.io/docs/core/entity)
diag_attr | Name of json attribute within diagnostic message payload
icon | From [MaterialDesign](https://materialdesignicons.com/) reference


Example - the device's IP address:
```
  ip.device_type = "sensor";
  ip.diag_attr = "wifi_ip";
  ip.icon = "mdi:ip-network";  
```

## 4B. Provide All Necessary Details to Build a Static Diagnostic Discovery Message ##

Implement **getDiscoveryMessage** that takes the **discovery_fact_diagnostic_metadata** you just created plus some additional details and returns a **discovery_config** structure. All these methods are already defined and you already defined the device details and topics, so *all you need to do is cut-and-paste this into your code*.
```
discovery_config getDiscoveryMessage(discovery_fact_diagnostic_metadata disc_meta){
    return getDiscoveryFactDiagnosticMessage( 
                                disc_meta, 
                                DEVICE_ID, 
                                buildShortDevicePayload( DEVICE_NAME, getMAC() ), 
                                AVAILABILITY_TOPIC, 
                                DIAGNOSTIC_TOPIC);
}
```

## 6B. Return Diagnostic Values ##

As per Step 3B, format your JSON payload like:
```
{
    "<diag_attr>": value,    
}
```

Example diagnostic payload:
```
{ "wifi_ip": "10.0.0.130" }
```

and publish it to the **DIAGNOSTIC_TOPIC** using:
```
void publish(String &topic, String &payload)
```

