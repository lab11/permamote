#! /usr/bin/python3

from PID import PID
from lifxlan import LifxLAN
import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqtt_publish
import json
import datetime

light_pid = PID(300, 0, 50)
light_pid.SetPoint = 100

hyster = 30
brightness = 0
last_seq = 0
max_achievable_lux = 150
step_size_percent = 5
duration = 5000

lan = LifxLAN()
light = lan.get_device_by_name("Neal's Light")
while light is None or light.get_service() != 1:
    light = lan.get_device_by_name("Neal's Light")
brightness = light.get_color()[2]
#light.set_brightness(brightness, 100)

def set_brightness(lux):
    global brightness
    #diff = setpoint - lux
    #step = diff/max_achievable_lux*65535
    light_pid.update(lux)
    print('pid output: ' + str(light_pid.output))
    brightness += light_pid.output
    if brightness > 65535:
        brightness = 65535
        light_pid.clear()
    elif brightness < 0:
        brightness = 0
        light_pid.clear()
    try:
        light.set_brightness(brightness, duration)
        print('brightness set to ' + str(brightness/65535*100) + ' percent at ' + str(datetime.datetime.now()))
    except:
        print('failed to set light brightness')

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("device/Permamote/c098e5110007")

def on_message(client, userdata, msg):
    #print(msg.topic+" "+str(msg.payload) + '\n')
    data = json.loads(msg.payload)
    seq_no = int(data['sequence_number'])
    # skip repeated sequence numbers
    global last_seq
    if seq_no == last_seq:
        return
    last_seq = seq_no
    lux = float(data['light_lux'])
    global brightness
    print('Current reading is %.02f lux, seq %d' % (lux, seq_no))
    set_brightness(lux)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("128.32.171.51")
client.loop_forever()
