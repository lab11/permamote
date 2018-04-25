#! /usr/bin/env python3

import os
import argparse
import importlib
import copy
import numpy as np
from simulate import simulate
import matplotlib.pyplot as plt
from glob import glob
from multiprocessing import Pool

configs = [
    # name, has battery, sec_conf, primary_conf
    ['Telos',       None,               'AA_battery_2'],
    ['Hamilton',    None,               'CR123A_battery'],
    ['BLEES',       None,               'Coin_battery'],
    ['Yerva',       'Yerva_cap',        None],
    ['Capybara_1',  'Capybara_cap_1',   None],
    ['Capybara_2',  'Capybara_cap_2',   None],
    ['Flicker',     'Flicker_cap',      None],
    ['EnHANTs',     'EnHANTs_battery',  None],
    ['DoubleDip',   'DoubleDip_battery',None],
    ['46',          '46_battery',       None],
    ['Pressac',     'Capybara_cap_1',   'CR123A_battery'],
    ['Permamote_i',  'LTO_battery',     None],
    ['Permamote',   'LTO_battery',      'Coin_battery'],
    ['Permamote',   'LTO_battery',      'CR123A_battery'],
    ['Permamote',   'LTO_battery',      'AA_battery_2'],
]
sec_configs = {
    'Yerva_cap': {
        'name' : 'secondary',
        'type' : 'capacitor',
        'esr_ohm' : 0.2,
        'capacity_J': 2*100E-6 * (3.3**2),
        'min_capacity_J': 2*100E-6 * (0.4**2),
        'leakage_power_W': 10E-9,
    },
    'Capybara_cap_1': {
        'name' : 'secondary',
        'type' : 'capacitor',
        'esr_ohm' : 100,
        'capacity_J': .5*(400E-6 + 330E-6 + 67.5E-3) * (3.3**2),
        'min_capacity_J': .5*(400E-6 + 330E-6 + 67.5E-3) * (0.4**2),
        'leakage_power_W': 100E-9,
    },
    'Capybara_cap_2': {
        'name' : 'secondary',
        'type' : 'capacitor',
        'esr_ohm' : 25,
        'capacity_J': .5*(300E-6 + 1100E-6 + 7.5E-3) * (3.3**2),
        'min_capacity_J': .5*(300E-6 + 1100E-6 + 7.5E-3) * (0.4**2),
        'leakage_power_W': 10E-9,
    },
    'Flicker_cap': {
        'name' : 'secondary',
        'type' : 'capacitor',
        'esr_ohm' : 0.2,
        'capacity_J': .5*(100+22+2.2+47+15+33)*1E-6 * (3.3**2),
        'min_capacity_J': .5*(100+22+2.2+47+15+33)*1E-6 * (0.4**2),
        'leakage_power_W': 10E-9,
    },
    'EnHANTs_battery': {
        'name' : 'secondary',
        'type' : 'battery',
        'esr_ohm' : 0.1,
        'capacity_mAh' : 150,
        'nominal_voltage_V' : 2.4,
        'leakage_constant': 1E5,
    },
    'DoubleDip_battery': {
        'name' : 'secondary',
        'type' : 'battery',
        'esr_ohm' : 1000,
        'capacity_mAh' : 45,
        'nominal_voltage_V' : 3.0,
        'leakage_constant': 1E5,
    },
    '46_battery': {
        'name' : 'secondary',
        'type' : 'battery',
        'esr_ohm' : 1000,
        'capacity_mAh' : 15,
        'nominal_voltage_V' : 3.0,
        'leakage_constant': 1E5,
    },
    'LTO_battery': {
        'name' : 'secondary',
        'type' : 'battery',
        'esr_ohm' : 0.1,
        'capacity_mAh' : 20,
        'nominal_voltage_V' : 2.4,
        'leakage_constant': 1E5,
    }
}

pri_configs = {
    'AA_battery_2': {
        'name' : 'primary',
        'capacity_mAh' : 2*2000,
        'nominal_voltage_V' : 1.5,
        'leakage_percent_year' : 1,
    },
    'CR123A_battery': {
        'name' : 'primary',
        'capacity_mAh' : 1550,
        'nominal_voltage_V' : 3.0,
        'leakage_percent_year' : 1,
    },
    'Coin_battery': {
        'name' : 'primary',
        'capacity_mAh' : 240,
        'nominal_voltage_V' : 3.0,
        'leakage_percent_year' : 1,
    },
    'Coin_battery_2': {
        'name' : 'primary',
        'capacity_mAh' : 480,
        'nominal_voltage_V' : 3.0,
        'leakage_percent_year' : 1,
    },
}


parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('-w', dest='workload', default='sense_and_send_workload.py', help='input workload config file e.g. `sense_and_send_workload.py`')
args = parser.parse_args()

# Make save folder if needed
save_dir = 'related/'
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

tracenames = ['../enhants/numpy_arrays/SetupA.npy',
        '../enhants/numpy_arrays/SetupC.npy',
        '../enhants/numpy_arrays/SetupD.npy']
tracename = tracenames[2]

conf_template = importlib.import_module('template_config').config()
workload = importlib.import_module(args.workload.split('.')[0]).workload()

data = []

lights = np.load(tracename)
fname = tracename.split('/')[-1].split('.')[0] + '_' + workload.config['name']
print(fname)
workload.dataset['filename'] = tracename
for config in configs:
    print(config[0])
    dev_config = copy.deepcopy(conf_template)
    if config[1] is not None: # has secondary
        sec_config = sec_configs[config[1]]
        dev_config.design_config['secondary'] = sec_config['name']
        dev_config.secondary_configs['secondary'] = sec_config
    else:
        pass
    if config[2] is not None: # has primary
        dev_config.design_config['intermittent'] = False
        dev_config.primary_config = pri_configs[config[2]]
    else:
        dev_config.design_config['intermittent'] = True

    lifetime, used, possible, missed, online, event_ttc = simulate(dev_config, workload, lights)
    # energy used
    used_e = used/possible
    # calculate average event_ttc, 95th percentile
    ttc_normal = event_ttc / workload.config['event_period_s']
    ttc_avg = np.average(ttc_normal)
    ttc_95 = np.percentile(ttc_normal, 95)
    # reliability
    events_successful = (missed.size - np.sum(missed))/missed.size
    online = np.sum(online)/online.size
    a = np.array([lifetime, used_e, events_successful, online, ttc_avg, ttc_95])
    print('lifetime: ' + str(lifetime))
    print('% used energy: ' + str(used_e))
    print('% events successful: ' + str(events_successful))
    print('% time online: ' + str(online))
    print('% ttc avg: ' + str(ttc_avg))
    print('% ttc 95: ' + str(ttc_95))
    print()
    data.append([config[0], a])

print(data)
data = np.asarray(data, dtype=object)
np.save(save_dir + fname, data)
