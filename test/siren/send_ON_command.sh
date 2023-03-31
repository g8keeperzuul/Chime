#!/bin/bash

# -----------------------------------------------------------------------------
source env.sh

# Calling the below service results in the MQTT publication of {state: ON, tone: alarm, duration: 10, volume_set: 1.0}
# Note that the data is 'volume_level', but the parameter is 'volume_set'.
# Volume is from 0.0 to 1.0
# The service will ONLY allow tones that have been pre-defined by available_tones:[doorbell,alarm] defined during discovery.
#
# service: siren.turn_on
# data:
#   tone: alarm
#   volume_level: 1
#   duration: "10"
# target:
#   entity_id: siren.featheresp32s2_chime

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/command" \
-u $USR -P $PWD \
-m "{\"state\":\"ON\", \"tone\": \"doorbell.wav\", \"volume_set\": 0.7 }"


# mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/availability" \
# -u $USR -P $PWD \
# -m "online"