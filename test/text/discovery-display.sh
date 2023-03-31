#!/bin/bash

# This is just a test. The device itself sends (multiple) discovery messages.

# -----------------------------------------------------------------------------
source env.sh

# NOTE: Recent (2023+) version of Home Assistant required to support Text integration
# !!! 2023-03-15 15:05:15.049 WARNING (MainThread) [homeassistant.components.mqtt.discovery] Integration text is not supported

# Text custom properties:
#   pattern: "regex"
#   mode: text|password
#   min:
#   max: (only up to 255)
#   encoding: utf-8

# .../command <<< {"text":"Basement smoke detector triggered!", "graphic": "FIRE" }
# .../command <<< {"text":"hello world", "graphic": "NONE" }  <<< "hello world" input from HA

# .../state   >>> {"text":"Basement smoke detector triggered!", "graphic": "FIRE" }
# .../state   >>> {"text":"hello world", "graphic": "NONE" }

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "${BASE_TOPIC}/config" \
-u $USR -P $PWD \
--retain \
-m "{\"entity_category\": \"config\",\
  \"device_class\": \"text\",\
  \"min\": \"0\",\
  \"max\": \"255\",\
  \"availability_topic\": \"${AVAIL_TOPIC}\",\
  \"unique_id\": \"featheresp32s2_display\",\
  \"device\": {\
    \"name\": \"Chime\",\
    \"identifiers\": \"84:F7:03:D6:8B:20\",\
    \"mf\": \"Adafruit\",\
    \"mdl\": \"Feather ESP32-S2 with BME280\",\
    \"sw\": \"20230302.2100\"},\
  \"name\": \"featheresp32s2 display\",\
  \"icon\": \"mdi:image-text\",\
  \"optimistic\": false,\
  \"state_topic\": \"${BASE_TOPIC}/state\",\
  \"value_template\": \"{{ value_json.text }}\",\
  \"command_topic\": \"${BASE_TOPIC}/command\",\
  \"command_template\": \"{ 'text': '{{ value }}', 'graphic': 'NONE' }\" }"

# Attempt to extract 'graphic' as text entity attribute
# works, but extracts both message and graphic as attributes:
# \"json_attributes_template\": \"{{ value_json | tojson }}\",\

# doesn't work:
# \"json_attributes_topic\": \"${BASE_TOPIC}/state\",\
# \"json_attributes_template\": \"{ 'graphic': '{{ value_json.graphic }}' }\",\