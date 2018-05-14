#! /usr/bin/python3

from PID import PID
from lifxlan import LifxLAN
import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqtt_publish
import json
import arrow, datetime
import time, threading
import operator

current_light = ''
devices = {}
state = 'baseline'
states = ['baseline', 'associating', 'done']
last_seq_no = {}

selected_labels = sorted([s + "'s light" for s in ['Neal', 'Will', 'Pat', 'Josh']])#['Neal', 'Pat', 'Josh', 'Will', 'Branden']])

#class permamote:
#    def __init__(self, device_id):
#        self.device_id = device_id


#def set_brightness(lux):
#    global brightness
#    try:
#        brightness = light.get_color()[2]
#    except:
#        pass
#    #diff = setpoint - lux
#    #step = diff/max_achievable_lux*65535
#    light_pid.update(lux)
#    print('pid output: ' + str(light_pid.output))
#    brightness += light_pid.output
#    if brightness > 65535:
#        brightness = 65535
#        light_pid.clear()
#    elif brightness < 0:
#        brightness = 0
#        light_pid.clear()
#    try:
#        light.set_brightness(brightness, duration)
#        print('light brightness set to ' + str(brightness/65535*100) + ' percent at ' + str(datetime.datetime.now()))
#    except:
#        print('failed to set light brightness')

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("device/Permamote/#")

def on_message(client, userdata, msg):
    #print(msg.topic)
    #print(msg.topic+" "+str(msg.payload) + '\n')
    data = json.loads(msg.payload)
    seq_no = int(data['sequence_number'])
    device_id = data['_meta']['device_id']
    if device_id not in last_seq_no:
        last_seq_no[device_id] = seq_no
    elif last_seq_no[device_id] == seq_no:
        return
    lux = float(data['light_lux'])
    if device_id not in devices:
        devices[device_id] = {}
    if state == 'baseline':
        devices[device_id]['baseline']= [lux, seq_no, -1]
    if state == 'associating':
        devices[device_id][current_light] = [lux, seq_no, 0]

# init mqtt
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("128.32.171.51")
client.loop_start()

# populate lights
lan = LifxLAN()
lights = []#lan.get_devices_by_name(selected_labels)
init_color = [0, 0, 65535, 4000]
for label in selected_labels:
    try:
        lights.append(lan.get_device_by_name(label))
    except:
        print('Failed to contact ' + label)

while(1):
    if state == 'baseline':
        print("starting baseline measurements!")
        # turn all lights off and get baseline light level for each
        for light in lights:
            light.set_color(init_color)
            #light.set_power(1)
            #time.sleep(1)
            light.set_power(0)
        # 20 seconds should be enough time to get messages from all motes
        time.sleep(60)
        state = 'associating'
    elif state == 'associating':
        print("starting association measurements!")
        for light in lights:
            current_light = light.label#light.mac_addr.replace(':','')
            try:
                light.set_power(1)
            except:
                print('Failed to set power ' + light.label)
            time.sleep(75)
            try:
                light.set_power(0)
            except:
                print('Failed to set power ' + light.label)
        for device in devices:
            try:
                baseline_lux = devices[device]['baseline'][0]
            except:
                print(device + ' does not have a baseline measurement')
            for light_source in devices[device]:
                if light_source == 'baseline': continue
                lux = devices[device][light_source][0]
                ratio = lux/baseline_lux
                devices[device][light_source][2] = ratio
            print(device)
            # sort lights by influence
            lights_sources_sorted = sorted(devices[device].items(), key=lambda x:x[1][2], reverse=True)
            for light_source in lights_sources_sorted:
                #if light_source[0] = 'baseline': continue
                print('\t' + light_source[0] + ' ' + str(light_source[1]))
        exit()
    else:
        pass
    time.sleep(0.5)
