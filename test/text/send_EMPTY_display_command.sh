#!/bin/bash
# Usage: $0 FIRE "Fire Alarm!"
# -----------------------------------------------------------------------------
source env.sh

# .../command <<< ""

mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "${BASE_TOPIC}/command" \
-u $USR -P $PWD \
-m ""


# mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$AVAIL_TOPIC" \
# -u $USR -P $PWD \
# -m "online"