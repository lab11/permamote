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
begin_time =    '07-01-2016 00:00:00 US/Eastern'
end_time =      '08-15-2016 00:00:00 US/Eastern'

tag_list = {
        'device_class': ['Blink'],
        #'location': ['4th_Floor_Hallway_outside_4908', '3rd_Floor', '545R'],
        'device_id': ['c098e5900023','c098e5900064'],
        }
group_list = ['location', 'device_class', 'device_id']
select_operation = 'value'
measurement_list = ['motion_last_minute']
out_filename = 'raw_data/motion'
generate_csv(config, select_operation, measurement_list, tag_list, begin_time, end_time, group_list,  out_filename)

# configurations
begin_time =    '02-01-2017 00:00:00 US/Pacific'
end_time =      '03-01-2017 00:00:00 US/Pacific'

tag_list = {
        'device_class': ['Blink'],
        #'location': ['4th_Floor_Hallway_outside_4908', '3rd_Floor', '545R'],
        'device_id': ['c098e5900084'],
        }
group_list = ['location', 'device_class', 'device_id']
select_operation = 'value'
measurement_list = ['motion_last_minute']
out_filename = 'raw_data/motion'
generate_csv(config, select_operation, measurement_list, tag_list, begin_time, end_time, group_list,  out_filename)

