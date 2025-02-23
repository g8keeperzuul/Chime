# This program subscribes to a number of topics with sensor data and formats that information 
# to be displayed on Chime by publishing a command message.
# It will cycle through each topic in order, displaying the data for a minimum time (MIN_DISPLAY_TIME_SEC).
# While a particular topic is actively being displayed, any real-time updates are also reflected on the display.
# When it cycles to the next topic, the lasted telemetry received will be displayed. 
#
# There is no timer for the display cycle. Instead the trigger is any time a message is received on any of the 
# subscribed topics (on_message()).
# This means if no messages are received on any of the subscribed topics, the display will not update or cycle. 

# -b BROKER_HOST -p BROKER_PORT -u USERNAME -s PASSWORD -v

# https://eclipse.dev/paho/files/paho.mqtt.python/html/index.html
import paho.mqtt.client as mqtt
import json
import time
import optparse

# Note: debug output will be delayed when using Docker since output is buffered
debug = False # override with -v

AIRMONITOR_SUB_TOPIC = "homeassistant/sensor/featherm0/state"
OUTSIDE_TEMP_SUB_TOPIC = "homeassistant/sensor/Acurite-Tower-2782/state"
RADIATIONWATCHER_SUB_TOPIC = "homeassistant/sensor/esp8266thing/state"

CHIME_CMD_PUB_TOPIC = "homeassistant/text/featheresp32s2/display/command"

placeholder_message = {"text":"Starting..", "graphic":"ALERT"}
display_index = 0
last_display_cycle_ts = 0
MIN_DISPLAY_TIME_SEC = 60

# Array of json messages containing the most up to date data received
latest_info = [placeholder_message, placeholder_message, placeholder_message]
AIRMONITOR_INDEX = 0
RADIATIONWATCHER_INDEX = 1
OUTSIDE_TEMP_INDEX = 2

# Callback when the client receives a message
def on_message(client, userdata, message):
   global latest_info, last_display_cycle_ts, display_index, debug
   payload = message.payload.decode('utf-8')
   data = json.loads(payload)
   
   if debug:
      print(f"on_message()-> {message.topic}")

   if message.topic == AIRMONITOR_SUB_TOPIC:

      temperature = data.get('temperature')
      humidity = data.get('humidity')
      pressure = data.get('pressure')
      carbondioxide = data.get('carbon_dioxide')
      aqi = data.get('aqi')

      if debug:
         print(f"Temperature: {temperature} °C")
         print(f"Humidity: {humidity} %")
         print(f"Pressure: {pressure} hPa")
         print(f"CO2: {carbondioxide} ppm")
         print(f"AQI: {aqi}")

      # Publish the formatted message
      latest_info[AIRMONITOR_INDEX] = {
         "text": f"{temperature} C\n{humidity} RH%\n{pressure} hPa\n\nCO2: {carbondioxide}\nAQI: {aqi}", "graphic": "MEDIUM_TEXT"
      }

   if message.topic == OUTSIDE_TEMP_SUB_TOPIC:
      temperature = data.get('temperature_C')
      humidity = data.get('humidity')

      if debug:
         print(f"Temperature: {temperature} °C")
         print(f"Humidity: {humidity} %")

      latest_info[OUTSIDE_TEMP_INDEX] = {
         "text": f"Outside\n\n{temperature} C\n{humidity} RH%\n", "graphic": "LARGE_TEXT"
      }

   if message.topic == RADIATIONWATCHER_SUB_TOPIC:
      cpm = data.get('frequency_details').get('cpm')

      if debug:
         print(f"CPM: {cpm}")

      latest_info[RADIATIONWATCHER_INDEX] = {
         "text": f"Gamma\nevents\n\n{cpm}\n    CPM", "graphic": "LARGE_TEXT"
      }

   # Has enough time passed to cycle the display to the next topic?
   if (time.time() - last_display_cycle_ts) > MIN_DISPLAY_TIME_SEC:
      # time to cycle the display to show information from a different topic...
      display_index = display_index + 1
      if display_index >= len(latest_info): # past end of list, reset to beginning...
         display_index = 0
      last_display_cycle_ts = time.time()

   # update display with latest data, even if it is not yet time to cycle...
   client.publish(CHIME_CMD_PUB_TOPIC, json.dumps(latest_info[display_index]))
   
   if debug:
      print(f"   last display cycle: {time.strftime('%H:%M:%S',time.localtime(last_display_cycle_ts))}")
      print(f"   last display index: {display_index}")


# def on_subscribe(client, userdata, mid, reason_codes, properties):
#     if isinstance(reason_codes, list):
#        for sub_result in reason_codes:
#           #if sub_result == 1:
#               # process QoS == 1
#           # Any reason code >= 128 is a failure.
#           if sub_result >= 128:
#              # error processing
#              print(f"Error subscribing to topic: {sub_result}")
#     else:
#        if reason_codes > 128:
#           print(f"Error subscribing to topic: {reason_codes}")

# def on_publish(client, userdata, mid, reason_codes, properties):
#     if isinstance(reason_codes, list):
#        for pub_result in reason_codes:
#           # Any reason code >= 128 is a failure.
#           if pub_result >= 128:
#              # error processing
#              print(f"Error publishing to topic: {pub_result}")
#     else:
#        if reason_codes > 128:
#           print(f"Error publishing to topic: {reason_codes}")


# Callback when the client connects to the broker
def on_connect(client, userdata, flags, reason_code, properties):
   if reason_code == 0:
      try:
         client.subscribe(AIRMONITOR_SUB_TOPIC)
         client.subscribe(OUTSIDE_TEMP_SUB_TOPIC)
         client.subscribe(RADIATIONWATCHER_SUB_TOPIC)
      except:    
         print(f"Subscription problem!")
         exit(-1)
   else:
      print(f"Connection issue with reason code: {reason_code}")
      exit(-2)

if __name__ == '__main__':
   parser = optparse.OptionParser()
   parser.add_option("-b", "--broker", action="store", type="string", dest="broker")
   parser.add_option("-p", "--port",   action="store", type="int",    dest="port", default=1883)
   parser.add_option("-u", "--user",   action="store", type="string", dest="username")
   parser.add_option("-s", "--pass",   action="store", type="string", dest="password")
   parser.add_option("-v",             action="store_true",           dest="verbose", default=False)
   opts, args = parser.parse_args()
   #process(args, broker=opts.broker, port=opts.port, user=opts.user, pass=opts.pass, verbose=opts.verbose)

   debug = opts.verbose

   if debug:
      print(f"MQTT broker: {opts.broker}:{opts.port}\nusername: {opts.username}, password: {opts.password}")

   # Set up the MQTT client
   #print(f"Paho MQTT version: {paho.mqtt.__version__[0]}")
   client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "mqtt2chime")
   client.on_connect = on_connect
   client.on_message = on_message
   # client.on_subscribe = on_subscribe
   # client.on_publish = on_publish

   # Connect to the MQTT broker (e.g., localhost or broker address)
   client.username_pw_set(username=opts.username, password=opts.password)
   client.connect(opts.broker, opts.port, 60)

   # Start the MQTT loop to process incoming messages
   client.loop_forever()

