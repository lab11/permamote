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
if not os.path.isfile('../confs/influxdb_old.conf'):
    print('Error: need a valid influxdb.conf file')
    sys.exit(1)

# hack to read sectionless ini files from:
#   http://stackoverflow.com/a/25493615/4422122
influxdb_config = configparser.ConfigParser()
with open('../confs/influxdb_old.conf', 'r') as f:
    config_str = '[global]\n' + f.read()
influxdb_config.read_string(config_str)
config = influxdb_config['global']

# configurations
measurement_list = ['light_lux', 'primary_voltage', 'secondary_voltage']
for measurement in measurement_list:
    begin_time =    '09-13-2018 00:00:00 US/Pacific'
    end_time   =    '11-30-2018 00:00:00 US/Pacific'

    tag_list = {
            #'device_class': ['Permamote'],
            'device_id': ['c098e5110003']
            }
    group_list = ['device_class', 'device_id']
    select_operation = '"value"'
    out_filename = 'exp_data/' + measurement
    generate_csv(config, select_operation, [measurement], tag_list, begin_time, end_time, group_list,  out_filename)

measurement = 'seq_no'
begin_time =    '09-13-2018 00:00:00 US/Pacific'
end_time   =    '11-30-2018 00:00:00 US/Pacific'

tag_list = {
        'device_class': ['Ligeiro'],
        'device_id': ['c098e5d0004a']
        }
group_list = ['device_class', 'device_id']
select_operation = '"value"'
out_filename = 'exp_data/' + measurement
generate_csv(config, select_operation, [measurement], tag_list, begin_time, end_time, group_list,  out_filename)

