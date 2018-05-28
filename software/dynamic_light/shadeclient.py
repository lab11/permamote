import requests
from shade_config import shade_config

class ShadeClient:
    def __init__(self, config):
        self.host = config['address'] + '/shades'
        self.user = config['user']
        self.password = config['password']
        self.battery = 0
        self.position = 0
    def get_battery(self, shade_mac):
        r = requests.request('GET', self.host, json = { 'shade': shade_mac, 'action': 'battery'}, auth = (self.user, self.password))
        self.battery = r.json()['battery']
        return self.battery
    def get_position(self, shade_mac):
        r = requests.request('GET', self.host, json = { 'shade': shade_mac, 'action': 'position'}, auth = (self.user, self.password))
        self.position = r.json()['position']
        return self.position
    def set_position(self, shade_mac, position):
        r = requests.request('POST', self.host, json = { 'shade': shade_mac, 'action': 'target', 'value': position}, auth = (self.user, self.password))

test = ShadeClient(shade_config)
battery = test.get_battery('F4:21:20:D9:FA:CD')
position = test.get_position('F4:21:20:D9:FA:CD')
print(battery, position)
test.set_position('F4:21:20:D9:FA:CD', 50)
