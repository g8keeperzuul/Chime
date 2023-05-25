#!/bin/bash

# -----------------------------------------------------------------------------
source env.sh

# .../command <<< {"text":"Basement smoke detector triggered!", "graphic":"FIRE"}

# MEDIUM_TEXT = 10 chars, 6 lines
# LARGE_TEXT = 7 chars, 5 lines
mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "${BASE_TOPIC}/command" \
-u $USR -P $PWD \
-m "{\"text\":\"  Carbon\n  Dioxide\n\n 1234 PPM\", \"graphic\": \"MEDIUM_TEXT\" }"
#-m "{\"text\":\"Carbon\nDioxide\n\n1234\n    PPM\", \"graphic\": \"LARGE_TEXT\" }"


# mosquitto_pub -h $MOSQUITTO_HOST -p $MOSQUITTO_PORT -t "$AVAIL_TOPIC" \
# -u $USR -P $PWD \
# -m "online"

# MEDIUM_TEXT:
#   1234567890
# 1 12.3 C
# 2 34.5 % RH
# 3 1234 hPa
# 4 
# 5 CO2: 1234
# 6 AQI: 1234
