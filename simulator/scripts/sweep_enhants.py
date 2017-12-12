#! /usr/bin/env python3

import argparse
from copy import deepcopy
from config import sim_config
from config import sweep_vars
import numpy as np
from simulate_enhants import simulate
import matplotlib.pyplot as plt
from glob import glob
from multiprocessing import Pool

# Input files for simulation
#parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
#parser.add_argument('lightfile', metavar='l', type=str, help='A file to use to simulate light conditions')
#args = parser.parse_args()

fnames = glob('../enhants/numpy_arrays/*')

def sweep(lightfile):
    # load light and motion files
    lights = np.load(lightfile)

    sim_config_default = sim_config()

    sweep_results = []
    for sweep_var in sweep_vars:
        # load default config
        sweep_config = deepcopy(sim_config_default)
        sweep_result = []
        for config in sweep_config.config_list:
            if config['name'] == sweep_var[0][0] and sweep_var[0][1] in config:
                sweep_design_parameter = sweep_var[0][1]
                sweep_design_component = config
                sweep_parameter_name = config['name'] + '_' + sweep_design_parameter
                sweep_range = sweep_var[1]
                break;

        for i in sweep_range:
            print()
            print(sweep_parameter_name + ': ' + str(i))
            sweep_design_component[sweep_design_parameter] = i
            lifetime, wasted, possible = simulate(sweep_config, lights)
            sweep_result.append([i, lifetime, wasted, possible])
            print('lasted ' + str(lifetime) + ' years')
            print('wasted ' + str(wasted) + ' Joules')
            print('could have harvested ' + str(possible) + ' Joules')
        sweep_result = np.array(sweep_result)
        sweep_results.append((sweep_parameter_name, sweep_result))

    for parameter_sweep in sweep_results:
        # plot lifetimes
        plt.figure()
        plt.grid(True, which='both', ls='-.', alpha=0.5)
        plt.xscale('log')
        plt.plot(parameter_sweep[1][:,0], parameter_sweep[1][:,1])
        titlestr = lightfile.split('/')[-1].split('.')[0] + ' lifetime vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
        plt.title(titlestr)
        plt.ylabel('lifetime (years)')
        plt.xlabel(' '.join(parameter_sweep[0].split('_')[1:]))
        plt.savefig(titlestr.replace(' ', '_'))

        # plot % energy utilized
        plt.figure()
        plt.grid(True, which='both', ls='-.', alpha=0.5)
        plt.xscale('log')
        wasted = parameter_sweep[1][:, 2]
        possible = parameter_sweep[1][:, 3]
        percent_used = (possible - wasted) / possible * 100
        plt.plot(parameter_sweep[1][:,0], percent_used)
        titlestr = lightfile.split('/')[-1].split('.')[0] + ' used percent of energy vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
        plt.title(titlestr)
        plt.ylabel('% of available harvested energy used')
        plt.xlabel(' '.join(parameter_sweep[0].split('_')[1:]))
        plt.savefig(titlestr.replace(' ', '_'))

p = Pool(len(fnames))
p.map(sweep, sorted(fnames))
