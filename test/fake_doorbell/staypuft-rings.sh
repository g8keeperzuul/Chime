#!/bin/bash

# Simulate when Zuul rings the doorbell.

# 1. Upload image
#
# File will be uploaded via Home Assistant Media Browser (which requires authentication)
# Assumes HA configured with "doorbell" as the media_dirs key (MEDIA_DIR_KEY):
#  homeassistant:
#     media_dirs:
#       doorbell: /config/www/doorbell-snapshots

./upload_doorbell_snapshot.sh staypuft.png


# 2. Display uploaded image
#
# The camera entity will be updated with the provided image. 
# Define camera to display snapshot (in HA configuration.yaml):
# homeassistant:
#      camera:    
#        - name: "Doorbell Snapshot"
#          platform: local_file
#          file_path: /config/www/doorbell-snapshot/placeholder.jpg     

./display_doorbell_snapshot.sh staypuft.png

# 3. Should trigger the "DOORBELL_ACTIVATED" automation
#
# Trigger based on ANY change to file file_path state of the camera.doorbell_snapshot entity

# 4. The automation action will call the "doorbell_activated" script
#
# This script will:
# 4.1 mqtt.publish '{"text":"Somebody is at the   front door", "graphic": "DOOR" }' to homeassistant/text/featheresp32s2/display/command
# 4.1.1 The device will reflect this command back to homeassistant/text/featheresp32s2/display/state
# 4.2 mqtt.publish '{"state":"ON", "tone": "doorbell.wav", "volume_set": 1.0 }' to homeassistant/siren/featheresp32s2/command
# 4.2.1 The device will reflect this command back to homeassistant/siren/featheresp32s2/state
# 4.3 notify
# 4.4 wait 1 minute
# 4.5 mqtt.publish '{"text":"", "graphic": "NONE" }' to homeassistant/siren/featheresp32s2/command
# 4.5.1 The device will reflect this command back to homeassistant/siren/featheresp32s2/state
