#! /usr/bin/env python3

import argparse
from config import sim_config
from config import sweep_vars
import numpy as np
from simulate import simulate
import matplotlib.pyplot as plt

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('lightfile', metavar='l', type=str, help='A file to use to simulate light conditions')
parser.add_argument('pirfile', metavar='p', type=str, help='A file to use to simulate wake up events from a PIR sensor')
args = parser.parse_args()

# load light and motion files
lights = np.load(args.lightfile)
motions = np.load(args.pirfile)

# load default config
sweep_config = sim_config

sweep_lifetimes = []
for sweep_var in sweep_vars:
    sweep_lifetime = []
    for config in sweep_config.config_list:
        if config['name'] == sweep_var[0][0] and sweep_var[0][1] in config:
            sweep_design_parameter = sweep_var[0][1]
            sweep_design_component = config
            sweep_parameter_name = config['name'] + '_' + sweep_design_parameter
            sweep_range = np.arange(sweep_var[1], sweep_var[2], sweep_var[3])
            break;

    print(sweep_parameter_name)
    for i in sweep_range:
        print()
        print(i)
        sweep_design_component[sweep_design_parameter] = i
        lifetime = simulate(sweep_config, lights, motions)
        sweep_lifetime.append([i, lifetime])
        print(str(lifetime) + ' years')
    sweep_lifetime = np.array(sweep_lifetime)
    sweep_lifetimes.append((sweep_parameter_name, sweep_lifetime))

for parameter_sweep in sweep_lifetimes:
    plt.plot(parameter_sweep[1][:,0], parameter_sweep[1][:,1])
    plt.show()
