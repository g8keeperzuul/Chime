#!/bin/bash

# -----------------------------------------------------------------------------
source env.sh

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/command" \
-u $USR -P $PWD \
-m "{\"state\":\"OFF\"}"

# mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/availability" \
# -u $USR -P $PWD \
# -m "online"