#!/bin/bash

# -----------------------------------------------------------------------------
source env.sh

# The following actions should be taken when the doorbell button is pressed.
# The will be captured as a Home Assistant Automation and Script

# 1. Publish siren command to play doorbell tone
# 2. Publish display command to show doorbell message


mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "homeassistant/siren/featheresp32s2/command" \
-u $USR -P $PWD \
-m "{\"state\":\"ON\", \"tone\": \"doorbell.wav\", \"volume_set\": 0.7 }"

# Supported Graphics: NONE, DOOR, RADS, FIRE, INFO, AQI, CO2, WATER, GAS, FREEZE, ALERT, SAFE, GARAGE
mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "homeassistant/text/featheresp32s2/display/command" \
-u $USR -P $PWD \
-m "{\"text\":\"Somebody is at the   front door\", \"graphic\": \"DOOR\" }"


# clear display
sleep 10s

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "homeassistant/text/featheresp32s2/display/command" \
-u $USR -P $PWD \
-m "{\"text\":\"\", \"graphic\": \"NONE\" }"
