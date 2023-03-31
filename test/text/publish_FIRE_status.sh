#!/bin/bash

# Only needed to simulate the device. 
# It should publish the state upon receiving a command.
# Device will reflect received command back to state topic

# -----------------------------------------------------------------------------
source env.sh


mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$BASE_TOPIC/state" \
-u $USR -P $PWD \
-m "{\"text\":\"Basement smoke detector triggered!\", \"graphic\": \"FIRE\" }"


mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$AVAIL_TOPIC" \
-u $USR -P $PWD \
-m "online"