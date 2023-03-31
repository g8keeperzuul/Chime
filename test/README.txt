To simulate to Home Assistant what messages the device sends to MQTT:

Run first

    Upon device setup(), publishes MQTT discovery message
    siren/discovery-chime.sh  <<< this simulates what the device would publish to MQTT 
    text/discovery-display.sh <<< this simulates what the device would publish to MQTT

Then 
  Siren:
    Device receives ON command (subscribes to homeassistant/siren/featheresp32s2/command topic), and 
    reflects ON status (publishes to homeassistant/siren/featheresp32s2/status topic).
    siren/send_ON_command.sh
    siren/publish_ON_status.sh <<< this simulates what the device would publish to MQTT

    Device receives OFF command (subscribes to homeassistant/siren/featheresp32s2/command topic), and 
    reflects OFF status (publishes to homeassistant/siren/featheresp32s2/status topic)
    siren/send_OFF_command.sh
    siren/publish_OFF_status.sh <<< this simulates what the device would publish to MQTT

  Text:
    Device receives command (subscribes to homeassistant/text/featheresp32s2/display/command topic), and 
    reflects status (publishes to homeassistant/text/featheresp32s2/display/status topic).
    text/send_FIRE_command.sh
    text/publish_FIRE_status.sh <<< this simulates what the device would publish to MQTT

    To turn off the display, the device receives the command (subscribes to homeassistant/text/featheresp32s2/display/command topic), and 
    reflects status (publishes to homeassistant/text/featheresp32s2/display/status topic).
    text/send_display_command.sh "NONE" ""


Home Assistant provides a service where the Siren can be turned ON/OFF or toggled

Calling the below service results in the MQTT publication of {state: ON, tone: alarm, duration: 10, volume_set: 1.0}
Note that the data is 'volume_level', but the parameter is 'volume_set'.
Volume is from 0.0 to 1.0
The service will ONLY allow tones that have been pre-defined by available_tones:[doorbell,alarm] defined during discovery.

service: siren.turn_on
data:
  tone: alarm
  volume_level: 1
  duration: "10"
target:
  entity_id: siren.featheresp32s2_chime


In configuration.yaml, re-label the default labels.

customize:
  number.featheresp32s2_refreshrate:
    friendly_name: Refresh Rate
  sensor.featheresp32s2_last_boot:
    friendly_name: Last Boot
  sensor.featheresp32s2_wifi_ip:
    friendly_name: IP Address
  sensor.featheresp32s2_wifi_mac:
    friendly_name: MAC Address
  sensor.featheresp32s2_wifi_rssi:
    friendly_name: Wifi RSSI
  siren.featheresp32s2_chime:
    friendly_name: Chime Audio
  text.featheresp32s2_display:
    friendly_name: Chime Display