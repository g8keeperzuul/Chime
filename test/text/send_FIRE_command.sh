#!/bin/bash

# -----------------------------------------------------------------------------
source env.sh

# .../command <<< {"text":"Basement smoke detector triggered!", "graphic":"FIRE"}

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "${BASE_TOPIC}/command" \
-u $USR -P $PWD \
-m "{\"text\":\"Basement smoke detector triggered!\", \"graphic\": \"FIRE\" }"


# mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$AVAIL_TOPIC" \
# -u $USR -P $PWD \
# -m "online"