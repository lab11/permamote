import requests
import http
from shade_config import shade_config

class ShadeClient:
    def __init__(self, config):
        self.host = config['address'] + '/shades'
        self.user = config['user']
        self.password = config['password']
    def get_battery(self, shade_mac):
        while(1):
            try:
                r = requests.request('GET', self.host, json = { 'shade': shade_mac, 'action': 'battery'}, auth = (self.user, self.password))
                break;
            except requests.exceptions.ConnectionError:
                pass
        battery = -1
        if 'battery' in r.json():
            battery = r.json()['battery']
        else:
            print(r.json())
        return battery
    def get_position(self, shade_mac):
        while(1):
            try:
                r = requests.request('GET', self.host, json = { 'shade': shade_mac, 'action': 'position'}, auth = (self.user, self.password))
                break;
            except requests.exceptions.ConnectionError:
                pass
        position = -1
        if 'position' in r.json():
            position = r.json()['position']
        else:
            print(r.json())
        return position
    def set_position(self, shade_mac, position):
        while(1):
            try:
                r = requests.request('POST', self.host, json = { 'shade': shade_mac, 'action': 'target', 'value': position}, auth = (self.user, self.password))
                break;
            except requests.exceptions.ConnectionError:
                pass

#test = ShadeClient(shade_config)
#battery = test.get_battery('F4:21:20:D9:FA:CD')
#position = test.get_position('F4:21:20:D9:FA:CD')
#print(battery, position)
#test.set_position('F4:21:20:D9:FA:CD', 50)
