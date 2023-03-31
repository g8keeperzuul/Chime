#!/bin/bash

# Only needed to simulate the device. 
# It should publish the state upon receiving a command.

# -----------------------------------------------------------------------------
source env.sh

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/state" \
-u $USR -P $PWD \
-m "{\"state\":\"ON\", \"tone\": \"alarm.mp3\", \"duration\": 10, \"volume_set\": 1.0 }"


mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/availability" \
-u $USR -P $PWD \
-m "online"