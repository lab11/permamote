#! /usr/bin/env python3

import os
import argparse
import importlib
from copy import deepcopy
import numpy as np
from simulate import simulate
import matplotlib.pyplot as plt
from glob import glob
from multiprocessing import Pool

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('-c', dest='config', default='permamote_config.py', help='input config file e.g. `permamote_config.py`')
parser.add_argument('-w', dest='workload', default='sense_and_send_workload.py', help='input workload config file e.g. `sense_and_send_workload.py`')
parser.add_argument('-s', dest='sweep', default='sec_size_sweep.py', help='input sweep config file e.g. `sec_size_sweep.py`')
parser.add_argument('-t', dest='title', help='description of run')
args = parser.parse_args()

#trace_to_use_list = ['SetupB']#'SetupD', 'SetupE']
#fnames = glob('../enhants/numpy_arrays/*')
#fname_keep = []
#for fname in fnames:
#    for tracename in trace_to_use_list:
#        if tracename in fname:
#            fname_keep.append(fname)
#fnames = fname_keep

# Make save folder if needed
save_dir = 'run_results/'
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

def sweep(config, workload, sweep):
    # load light
    trace_fname = workload.dataset['filename']
    lights = np.load(trace_fname)
    if args.title:
        workload_modifier = args.title
    elif 'period_s' in sweep.sweep_vars[0][0]:
        workload_modifier = str(config.secondary_configs[config.design_config['secondary']]['capacity_J'])
    elif workload.config['type'] == 'periodic' or workload.config['type'] == 'random':
        workload_modifier = str(workload.config['period_s'])
    else:
        workload_modifier = str(workload.config['lambda'])
    description = trace_fname.split('/')[-1].split('.')[0] + ' ' + ' '.join(workload.config['name'].split('_')) + ' ' + workload_modifier
    print(description)

    sweep_results = []
    for sweep_var in sweep.sweep_vars:
        # load default config
        sweep_config = deepcopy(config)
        sweep_result = []
        for s_config in sweep_config.config_list:
            if s_config['name'] == sweep_var[0][0] and sweep_var[0][1] in s_config:
                sweep_design_parameter = sweep_var[0][1]
                sweep_design_component = s_config
                sweep_parameter_name = s_config['name'] + '_' + sweep_design_parameter
                sweep_range = sweep_var[1]
                break;

        if workload.config['name'] == sweep_var[0][0] and sweep_var[0][1] in workload.config:
           sweep_design_parameter = sweep_var[0][1]
           sweep_design_component = workload.config
           sweep_parameter_name = workload.config['name'] + '_' + sweep_design_parameter
           sweep_range = sweep_var[1]

        for i in sweep_range:
            print(sweep_parameter_name + ': ' + str(i))
            sweep_design_component[sweep_design_parameter] = i
            lifetime, used, possible, missed, online, event_ttc = simulate(sweep_config, workload, lights)
            energy_used = used/possible
            events_successful = (missed.size - np.sum(missed))/missed.size
            time_online = np.sum(online) / online.size
            ttc = event_ttc#np.average(event_ttc)/ workload.config['event_period_s']
            sweep_result.append([i, lifetime, energy_used, events_successful, time_online, ttc])
            print("%d%% lifetime" % lifetime)
            print("%.2f%% Joules used" % (100*energy_used))
            print("%.2f%% events successful" % (100*events_successful))
            print("%.2f%% of time online" % (100*time_online))
            print("%.2f%% x expected event time to completion" % (np.average(event_ttc)/workload.config['event_period_s']))

        sweep_result = np.array(sweep_result, dtype='object')
        sweep_results.append((sweep_parameter_name, sweep_result))

    for sweep_var, parameter_sweep in zip(sweep.sweep_vars, sweep_results):
        x = parameter_sweep[1]
        titlestr = trace_fname.split('/')[-1].split('.')[0] + ' ' + ' '.join(parameter_sweep[0].split('_')[:-1]) + ' ' + ' '.join(workload.config['name'].split('_')) + ' ' + workload_modifier
        print(titlestr)
        np.save(save_dir + titlestr.replace(' ', '_'), x)
        #if sweep_var[2] == 'both' or sweep_var[2] == 'life':
        #    titlestr = lightfile.split('/')[-1].split('.')[0] + ' lifetime vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
        #    np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, lifetimes), axis=1))
        #    if (args.plot):
        #        # plot lifetimes
        #        plt.figure()
        #        plt.grid(True, which='both', ls='-.', alpha=0.5)
        #        #plt.xscale('log')
        #        plt.plot(x, lifetimes)
        #        plt.title(titlestr)
        #        plt.ylabel('lifetime (years)')
        #        xlabelstr = ' '.join(parameter_sweep[0].split('_')[1:-1]) + ' (' + parameter_sweep[0].split('_')[-1] + ')'
        #        plt.xlabel(xlabelstr)
        #        plt.savefig(save_dir + titlestr.replace(' ', '_'))
        #if sweep_var[2] == 'both' or sweep_var[2] == 'util':
        #    titlestr = lightfile.split('/')[-1].split('.')[0] + ' % energy usage vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
        #    np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, percent_used), axis=1))
        #    if (args.plot):
        #        # plot % energy utilized
        #        plt.figure()
        #        plt.grid(True, which='both', ls='-.', alpha=0.5)
        #        plt.xscale('log')
        #        plt.plot(x, percent_used)
        #        plt.title(titlestr)
        #        plt.ylabel('% of available harvested energy used')
        #        plt.xlabel(' '.join(parameter_sweep[0].split('_')[1:]))
        #        plt.savefig(save_dir + titlestr.replace(' ', '_'))

print('starting sweep')
imported_config = importlib.import_module(args.config.split('.')[0]).config()
imported_workload = importlib.import_module(args.workload.split('.')[0]).workload()
imported_sweep = importlib.import_module(args.sweep.split('.')[0]).sweep()

sweep(imported_config, imported_workload, imported_sweep)
