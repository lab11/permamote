import lifxlan
from lifxlan import LifxLAN
import paho.mqtt.client as mqtt
from PID import PID
from enum import Enum
import numpy as np
import time
import pickle
import json
from lightsensor import LightSensor
import os.path
import datetime

class LightControl:
    class State(Enum):
        IDLE = 0
        DISCOVER = 1
        CHARACTERIZE = 2
        CONTROL = 3

    def __init__(self, mqtt_address):
        self.state = self.State.IDLE

        self.lights = []
        self.sensors = {}
        self.sensors_to_lights = {}
        self.shades = []
        # keep track of sensors seen during
        # characterization steps
        self.seen_sensors = set()
        self.current_light = None
        self.current_brightness = None

        self.pid_controllers = {}

        self.lan = LifxLAN()

        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message

        self.mqtt_client.connect(mqtt_address)
        self.mqtt_client.loop_start()

    def discover(self, light_list, sensor_list, shade_list):
        # discover lights, shades, sensors
        have_file = os.path.isfile('sensor_light_mappings.pkl') and os.path.getsize('sensor_light_mappings.pkl') > 0
        if light_list is None and not have_file:
            while(1):
                try:
                    self.lights = self.lan.get_lights()
                    for light in self.lights:
                        light.get_label()
                        print('discovered ' + light.label)
                    break
                except lifxlan.errors.WorkflowException:
                    print('failed to get light')
        else:
            known_lights = []
            if have_file:
                with open('sensor_light_mappings.pkl', 'rb') as f:
                    self.sensors_to_lights = pickle.load(f)
                for sensor in self.sensors_to_lights:
                    if self.sensors_to_lights[sensor] is not None:
                        known_lights.append(self.sensors_to_lights[sensor])
            else:
                known_lights = light_list
            for label in known_lights:
                while(1):
                    try:
                        light = self.lan.get_device_by_name(label)
                    except lifxlan.errors.WorkflowException as e:
                        print('failed to discover ' + label)
                        print(e)
                        time.sleep(1)
                        continue
                    if light is None:
                        print('failed to get ' + label)
                        print('\tlight is None')
                        time.sleep(1)
                    else:
                        self.lights.append(light)
                        print('discovered ' + light.label)
                        break
        for light in self.lights:
            pid = PID(300, 1, 5)
            pid.SetPoint = 150
            if light.label is None:
                try:
                    light.get_label()
                except lifxlan.errors.WorkflowException:
                    print('could not get label, somethings messed up')
                    exit()
            self.pid_controllers[light.label] = pid


            # turn all lights off
            while(1):
                try:
                    init_color = list(light.get_color())
                    init_color[-1] = 4000
                    #init_color = [0, 0, 0, 4000]
                    light.set_color(init_color)
                    light.set_power(0)
                    break
                except lifxlan.errors.WorkflowException:
                    print(light.label + ' failed to set up light color for discovery')
        # shade discover is kind of dumb right now
        self.shades = shade_list
        # start sensor discovery
        self.state = self.State.DISCOVER
        # sleep for some time to allow discovery
        # BLE sucks sometimes, eh?
        time.sleep(20)
        # TODO pick sensors out of saved data
        print('Discovered the following sensors:')
        for sensor in self.sensors:
            print('\t' + sensor)

    def characterize(self):

        if len(self.sensors_to_lights) == 0:
            self.state = self.State.CHARACTERIZE
            print("starting characterizing measurements!")
            for light in self.lights:
                label = light.label
                if label is None:
                    print('this light is wack!')
                    print(light)
                    continue
                self.current_light = label
                print(label)
                # turn on the light
                if(light.power == 0):
                    while(1):
                        try:
                            light.set_power(1)
                            break
                        except lifxlan.errors.WorkflowException:
                            print('Failed to turn on light ' + label)
                # sweep through brightness
                for brightness in range(0, 101, 10):
                    print(brightness)
                    #for device in self.sensors:
                    #    print(self.sensors[device])
                    # clear seen devices
                    self.seen_sensors.clear()
                    #print(brightness)
                    self.current_brightness = brightness
                    while(1):
                        try:
                            light.set_brightness(brightness/100*65535)
                            break
                        except lifxlan.errors.WorkflowException:
                            print('Failed to set brightness ' + label)
                    while(1):
                        #print(sorted(self.seen_sensors))
                        #print(sorted([x for x in self.sensors.keys()]))
                        #print()
                        if self.seen_sensors == set([x for x in self.sensors.keys()]):
                            break
                        time.sleep(5)
                try:
                    light.set_power(0)
                except lifxlan.errors.WorkflowException:
                    print('Failed to turn off light' + label)
            for device in self.sensors:
                baseline_lux = self.sensors[device].baseline
                for light in self.sensors[device].light_char_measurements:
                    measurements = np.array(self.sensors[device].light_char_measurements[light])
                    measurements[:, 1] -= baseline_lux
                    ind = measurements[:, 1] < 0
                    measurements[:, 1][ind] = 0
                    self.sensors[device].light_char_measurements[light] = measurements
                # save because why not?
                with open(device + '_characterization.pkl', 'wb') as output:
                    pickle.dump(self.sensors[device].light_char_measurements, output, pickle.HIGHEST_PROTOCOL)



            # generate primary light associations
            # eventually we can do smarter things than 1-to-1 mappings
            for device in self.sensors:
                #print(self.sensors[device])
                max_affect_light = (None, 50)
                for light in self.sensors[device].light_char_measurements:
                    max_effect = self.sensors[device].light_char_measurements[light][-1][1]
                    if max_effect > max_affect_light[1]:
                        max_affect_light = (light, max_effect)
                self.sensors_to_lights[device] = max_affect_light[0]
            print(self.sensors_to_lights)
            with open('sensor_light_mappings.pkl', 'wb') as output:
                pickle.dump(self.sensors_to_lights, output, pickle.HIGHEST_PROTOCOL)

        self.state = self.state.CONTROL

        self.current_light = None
        self.current_brightness = None
        time.sleep(2)

    # mqtt source for sensor data
    def on_connect(self, client, userdata, flags, rc):
        print("Connected to mqtt stream with result code "+str(rc))
        client.subscribe("device/Permamote/#")
        client.subscribe("device/BLEES/#")

    def on_message(self, client, userdata, msg):
        #print(msg.topic+" "+str(msg.payload) + '\n')

        data = json.loads(msg.payload)
        seq_no = int(data['sequence_number'])
        device_id = data['_meta']['device_id']
        lux = float(data['light_lux'])

        # state machine for message handling
        if self.state == self.State.IDLE:
            return
        elif self.state == self.State.DISCOVER:
            if device_id not in self.sensors:
                self.sensors[device_id] = LightSensor(device_id)
            self.sensors[device_id].baseline = lux
            self.sensors[device_id].update_seq_no(seq_no)

        elif self.state == self.State.CHARACTERIZE:
            # only consider devices we've done baseline measurements for
            #print('Saw ' + str(device_id))
            if self.current_light is None:
                # if the current light is not set yet, ignore this
                print('huh?')
                return
            if device_id not in self.sensors:
                print("Saw sensor not in devices with baseline measurement: " + str(device_id))
                return
            #if device_id not in self.seen_sensors:
            self.seen_sensors.add(device_id)
            #print(self.seen_sensors)
            if self.current_light not in self.sensors[device_id].light_char_measurements:
                self.sensors[device_id].light_char_measurements[self.current_light] = []
            measurements = [x[0] for x in self.sensors[device_id].light_char_measurements[self.current_light]]
            if self.current_brightness not in measurements:
                self.sensors[device_id].light_char_measurements[self.current_light].append([self.current_brightness, lux])
        elif self.state == self.State.CONTROL:
            self.sensors[device_id].lux = lux

    def update_lights(self):
        for sensor in self.sensors_to_lights:
            label = self.sensors_to_lights[sensor]
            if label is None:
                #print('oh shoot this sensor does not have a light association')
                continue
            print('updating ' + label + ' and ' + sensor)
            light_to_update = None
            for light in self.lights:
                if light.label is None:
                    try:
                        light.get_label()
                    except lifxlan.errors.WorkflowException:
                        print('could not get label, somethings messed up')
                        continue
                elif light.label == label:
                    light_to_update = light
                    break
            if light_to_update is None:
                print('this light is wonky')
                continue
            if light_to_update.power_level == 0:
                try:
                    light_to_update.set_power(1)
                except lifxlan.errors.WorkflowException:
                    print('Failed to turn on light ' + label)
            pid = self.pid_controllers[light_to_update.label]
            try:
                color = light_to_update.get_color()
                brightness = color[2]
            except lifxlan.errors.WorkflowException:
                print('Failed to get brightness')
                continue
            pid.update(self.sensors[sensor].lux)
            print(brightness)
            print(self.sensors[sensor].lux)
            print(pid.output)
            brightness += pid.output
            if brightness > 65535:
                brightness = 65535
            elif brightness < 0:
                brightness = 0
            try:
                light_to_update.set_brightness(brightness, 5000)
                print(label + ' brightness set to ' + str(brightness/65535*100) + ' percent at ' + str(datetime.datetime.now()))
            except lifxlan.errors.WorkflowException:
                print(label + ' failed to set light brightness')
            print()
        print()

    def control_loop(self):
        time.sleep(5)


shade_list = [
        "F4:21:20:D9:FA:CD",
        "DF:93:08:12:38:CF",
        "FD:97:B5:16:08:4F",
        "FD:97:B5:16:08:4F",
        "F0:8A:FB:BC:E9:82",
        ]
selected_labels = sorted([s + "'s light" for s in ['Neal', 'Pat', 'Josh', 'Will']])
#selected_labels = ["Neal's light"]

#lan = LifxLAN()
#lights = lan.get_lights()
#for light in lights:
#    print(light)
#exit()

lightcontrol = LightControl("128.32.171.51")
lightcontrol.discover(None, None, shade_list)
#print(lightcontrol.lights)
#print(sorted(lightcontrol.sensors.keys()))
lightcontrol.characterize()
while(1):
    lightcontrol.update_lights()
    time.sleep(10)
