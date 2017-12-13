#! /usr/bin/env python3

import os
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
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('-p', dest='plot', action='store_true', default=False, help='plot data instead of putting into numpy array')
args = parser.parse_args()

trace_to_use_list = ['SetupB', 'SetupD', 'SetupE']
fnames = glob('../enhants/numpy_arrays/*')
fname_keep = []
for fname in fnames:
    for tracename in trace_to_use_list:
        if tracename in fname:
            fname_keep.append(fname)
fnames = fname_keep

# Make save folder if needed
save_dir = 'run_results/'
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

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
            print(sweep_parameter_name + ': ' + str(i))
            sweep_design_component[sweep_design_parameter] = i
            lifetime, wasted, possible = simulate(sweep_config, lights)
            sweep_result.append([i, lifetime, wasted, possible])
            print('lasted ' + str(lifetime) + ' years')
            print('wasted ' + str(wasted) + ' Joules')
            print('could have harvested ' + str(possible) + ' Joules\n')
        sweep_result = np.array(sweep_result)
        sweep_results.append((sweep_parameter_name, sweep_result))

    for sweep_var, parameter_sweep in zip(sweep_vars, sweep_results):
        x = parameter_sweep[1][:, 0]
        lifetimes = parameter_sweep[1][:, 1]
        wasted = parameter_sweep[1][:, 2]
        possible = parameter_sweep[1][:, 3]
        percent_used = (possible - wasted) / possible * 100
        if sweep_var[2] == 'both' or sweep_var[2] == 'life':
            titlestr = lightfile.split('/')[-1].split('.')[0] + ' lifetime vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
            np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, lifetimes), axis=1))
            if (args.plot):
                # plot lifetimes
                plt.figure()
                plt.grid(True, which='both', ls='-.', alpha=0.5)
                plt.xscale('log')
                plt.plot(x, lifetimes)
                plt.title(titlestr)
                plt.ylabel('lifetime (years)')
                xlabelstr = ' '.join(parameter_sweep[0].split('_')[1:-1]) + ' (' + parameter_sweep[0].split('_')[-1] + ')'
                plt.xlabel(xlabelstr)
                plt.savefig(save_dir + titlestr.replace(' ', '_'))
        if sweep_var[2] == 'both' or sweep_var[2] == 'util':
            titlestr = lightfile.split('/')[-1].split('.')[0] + ' % energy usage vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
            np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, percent_used), axis=1))
            if (args.plot):
                # plot % energy utilized
                plt.figure()
                plt.grid(True, which='both', ls='-.', alpha=0.5)
                plt.xscale('log')
                plt.plot(x, percent_used)
                plt.title(titlestr)
                plt.ylabel('% of available harvested energy used')
                plt.xlabel(' '.join(parameter_sweep[0].split('_')[1:]))
                plt.savefig(save_dir + titlestr.replace(' ', '_'))

#sweep(sorted(fnames)[0])
p = Pool(len(fnames))
p.map(sweep, sorted(fnames))
