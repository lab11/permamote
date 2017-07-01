#! /usr/bin/env python3

import os
import sys

try:
    import configparser
except ImportError:
    print('Could not import configparser library')
    print('sudo pip3 install configparser')
    sys.exit(1)

# import influx to csv library
sys.path.append('../influx-query/influx_to_csv/')
print(sys.path)
from influx_to_csv import generate_csv

# check for influxdb config
if not os.path.isfile('../confs/influxdb.conf'):
    print('Error: need a valid influxdb.conf file')
    sys.exit(1)

# hack to read sectionless ini files from:
#   http://stackoverflow.com/a/25493615/4422122
influxdb_config = configparser.ConfigParser()
with open('../confs/influxdb.conf', 'r') as f:
    config_str = '[global]\n' + f.read()
influxdb_config.read_string(config_str)
config = influxdb_config['global']

# configurations
begin_time =    '08-01-2016 00:00:00 US/Eastern'
end_time =      '10-01-2016 00:00:00 US/Eastern'

tag_list = {
        'location': ['4908', '3941', '3725', '4901'],
        'device_class': ['BLEES'],
        'device_id': ['c098e530000c', 'c098e530003a', 'c098e5300069', 'c098e5300065', 'c098e530007d'],
        }
group_list = ['location', 'device_class', 'device_id', 'time(1m) fill(0)']
select_operation = 'MEAN("value")'
measurement_list = 'light_lux'
out_filename = 'raw_data/light'
generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)

tag_list = {
        'device_id': [  'c098e590009d', 'c098e5900060', 'c098e5900039',
                        'c098e59000a1', 'c098e5900044', 'c098e59000a0',
                        'c098e5900089', 'c098e590007d'
                     ],
        'device_class': ['Blink'],
        }
group_list = ['location', 'device_class', 'device_id', "time(1m) fill(0)"]
select_operation = 'MODE(value)'
measurement_list = 'motion_last_minute'
out_filename = 'raw_data/motion'
generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)

