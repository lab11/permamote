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
begin_time =    '01-01-2015 00:00:00 US/Eastern'
end_time =      '07-01-2017 00:00:00 US/Eastern'

tag_list = {
        'device_class': ['BLEES'],
        'location': ['2725', '2733', '3901', '3941', '4908', '4941'],
        #'device_id': ['c098e530006a', 'c098e530006b', 'c098e5300069', 'c098e5300072', 'c098e5300065', 'c098e530003a'],
        }
group_list = ['location', 'device_class', 'device_id']
select_operation = 'value'
measurement_list = 'light_lux'
out_filename = 'raw_data/light'
generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)

tag_list = {
        'device_class': ['Blink'],
        'location': ['2725', '2733', '3901', '3941', '4908', '4941'],
        #'device_id':[   'c098e5900092', 'c098e5900029', 'c098e590009a',
        #                'c098e590002a', 'c098e5900093', 'c098e5900021',
        #                'c098e590009d', 'c098e5900060', 'c098e5900020'
        #            ],
        }
group_list = ['location', 'device_class', 'device_id']
select_operation = 'value'
measurement_list = 'motion_last_minute'
out_filename = 'raw_data/motion'
generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)

#tag_list = {
#        'device_class': ['Blink'],
#        'location': ['2725', '2733', '3901', '3941', '4908', '4941'],
#        'device_id':[
#                        'c098e590009e', 'c098e5900006', 'c098e5900095',
#                        #'c098e5900097',
#                    ],
#        }
#generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)
#
#tag_list = {
#        'device_class': ['Blink'],
#        'location': ['2725', '2733', '3901', '3941', '4908', '4941'],
#        #'device_id':[
#        #                'c098e5900099', 'c098e590009f', #'c098e5900008', #'c098e590000d',
#        #            ],
#        }
##generate_csv(config, select_operation, measurement_list, tag_list, group_list, begin_time, end_time, out_filename)
