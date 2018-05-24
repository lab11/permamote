#! /usr/bin/python3

from PID import PID
from lifxlan import LifxLAN
import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqtt_publish
import json
import arrow, datetime
import time, threading
import operator
from pprint import pprint
from matplotlib import pyplot as plot
import numpy as np
import pickle

current_light = None
current_brightness = 0
devices = {}
seen_devices = []
state = 'baseline'
states = ['baseline', 'associating', 'done']
last_seq_no = {}

selected_labels = sorted([s + "'s light" for s in ['Neal', 'Will', 'Pat', 'Josh']])#['Neal', 'Pat', 'Josh', 'Will', 'Branden']])
#selected_labels = sorted([s + "'s light" for s in ['Neal']])#['Neal', 'Pat', 'Josh', 'Will', 'Branden']])

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
    #client.subscribe("device/Permamote/c098e5110007")
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

    if state == 'baseline':
        if device_id not in devices:
            devices[device_id] = {}
        devices[device_id]['baseline']= lux
    if state == 'characterizing':
        # only consider devices we've done baseline measurements for
        #print('Saw ' + str(device_id))
        if device_id not in devices:
            print("Saw device not in devices with baseline measurement: " + str(device_id))
            return
        if device_id not in seen_devices:
            #print("Hadn't seen it in this round")
            seen_devices.append(device_id)
        if current_light == None:
            return
        elif current_light not in devices[device_id]:
            devices[device_id][current_light] = []
        measurements = [x[0] for x in devices[device_id][current_light]]
        if current_brightness not in measurements:
            devices[device_id][current_light].append([current_brightness, lux])

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
        if lights[-1] == None: raise ValueError('Failed to get device')
    except:
        print('Failed to contact ' + label)
        exit()

while(1):
    if state == 'baseline':
        print("starting baseline measurements!")
        # turn all lights off and get baseline light level for each
        for light in lights:
            light.set_color(init_color)
            #light.set_power(1)
            #time.sleep(1)
            light.set_power(0)
        # 30 seconds should be enough time to get messages from all motes
        time.sleep(10)
        state = 'characterizing'
    elif state == 'characterizing':
        print("starting characterizing measurements!")
        for light in lights:
            print(light.label)
            current_light = light.label
            # turn on the light
            while(1):
                try:
                    light.set_power(1)
                    break;
                except:
                    print('Failed to turn on light ' + light.label)
            # sweep through brightness
            for brightness in range(0, 101, 10):
                # clear seen devices
                seen_devices = []
                pprint(devices)
                #print(brightness)
                current_brightness = brightness
                while(1):
                    try:
                        light.set_brightness(brightness/100*65535)
                        break;
                    except:
                        print('Failed to set brightness ' + light.label)
                while(1):
                    #print(sorted(seen_devices))
                    #print(sorted([x for x in devices.keys()]))
                    #print()
                    if sorted(seen_devices) == sorted([x for x in devices.keys()]):
                        break;
                    time.sleep(5)
            try:
                light.set_power(0)
            except:
                print('Failed to turn off light' + light.label)
        for device in devices:
            try:
                baseline_lux = devices[device]['baseline']
            except:
                print(device + ' does not have a baseline measurement')
            for light_source in devices[device]:
                if light_source not in selected_labels: continue
                measurements = np.array(devices[device][light_source])
                measurements[:, 1] -= baseline_lux
                devices[device][light_source] = measurements
        pprint(devices)
        with open('device_characterization.pkl', 'wb') as output:
            pickle.dump(devices, output, pickle.HIGHEST_PROTOCOL)
        exit()
            # sort lights by influence
            #lights_sources_sorted = sorted(devices[device].items(), key=lambda x:x[1][0], reverse=True)
            #for light_source in lights_sources_sorted:
            #    #if light_source[0] = 'baseline': continue
            #    print('\t' + light_source[0] + ' ' + str(light_source[1]))
    else:
        pass
    time.sleep(0.5)
