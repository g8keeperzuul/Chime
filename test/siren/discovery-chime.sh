#!/bin/bash

# This is just a test. The device itself sends (multiple) discovery messages.


# -----------------------------------------------------------------------------
source env.sh

# .../state   >>> {"state":"ON", "tone": "doorbell", "volume_set": 0.5, "duration": 3 }
# .../state   >>> {"state":"OFF" }
# .../command <<< {"state":"ON", "tone": "doorbell", "volume_set": 0.5, "duration": 3 }
# .../command <<< {"state":"OFF"}
mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "${BASE_TOPIC}/chime/config" \
-u $USR -P $PWD \
--retain \
-m "{\"entity_category\": \"config\",\
  \"device_class\": \"siren\",\
  \"availability_topic\": \"${BASE_TOPIC}/availability\",\
  \"unique_id\": \"featheresp32s2_chime\",\
  \"device\": {\
    \"name\": \"Chime\",\
    \"identifiers\": \"84:F7:03:D6:8B:20\",\
    \"mf\": \"Adafruit\",\
    \"mdl\": \"Feather ESP32-S2 with BME280\",\
    \"sw\": \"20230302.2100\"},\
  \"name\": \"featheresp32s2 chime\",\
  \"icon\": \"mdi:bullhorn\",\
  \"support_duration\": true,\
  \"support_volume_set\": true,\
  \"available_tones\":[\"DOORBELL\",\"ALARM\"],\
  \"optimistic\": true,\
  \"state_topic\": \"${BASE_TOPIC}/state\",\
  \"state_value_template\": \"{{ value_json.state }}\",\
  \"state_on\": \"ON\",\
  \"state_off\": \"OFF\",\
  \"command_topic\": \"${BASE_TOPIC}/command\",\
  \"command_template\": \"{ 'state': '{{ value }}', 'tone': '{{ tone }}', 'volume_set': {{ volume_level }}, 'duration': {{ duration }} }\",\
  \"command_off_template\": \"{ 'state': '{{ value }}' }\",\
  \"payload_on\": \"ON\",\
  \"payload_off\": \"OFF\" }"

