#! /usr/bin/env python3

import os
import pickle
import argparse
import yaml
from copy import deepcopy
import numpy as np
import pandas as pd
from simulate import EHSim
import matplotlib.pyplot as plt
from glob import glob
from multiprocessing import Pool

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('-c', dest='config_file', default='config.yaml', help='input config file e.g. `config.yaml`')
parser.add_argument('-w', dest='workload_file', default='workload.yaml', help='input workload config file e.g. `workload.yaml`')
parser.add_argument('-d', dest='dataset_file', default='dataset.yaml', help='input dataset config file e.g. `dataset.yaml`')
parser.add_argument('-s', dest='sweep_file', default='sweep.yaml', help='input sweep config file e.g. `sweep.yaml`')
args = parser.parse_args()

# Make save folder if needed
save_dir = 'run_results/'
if not os.path.exists(save_dir):
    os.makedirs(save_dir)

#def sweep(config, workload, sweep):
#    # load light
#    trace_fname = workload.dataset['filename']
#    lights = np.load(trace_fname)
#    if args.title:
#        workload_modifier = args.title
#    elif 'period_s' in sweep.sweep_vars[0][0]:
#        workload_modifier = str(config.secondary_configs[config.design_config['secondary']]['capacity_J'])
#    elif workload.config['type'] == 'periodic' or workload.config['type'] == 'random':
#        workload_modifier = str(workload.config['period_s'])
#    else:
#        workload_modifier = str(workload.config['lambda'])
#    description = trace_fname.split('/')[-1].split('.')[0] + ' ' + ' '.join(workload.config['name'].split('_')) + ' ' + workload_modifier
#    print(description)
#
#    sweep_results = []
#    for sweep_var in sweep.sweep_vars:
#        # load default config
#        sweep_config = deepcopy(config)
#        sweep_result = []
#        for s_config in sweep_config.config_list:
#            if s_config['name'] == sweep_var[0][0] and sweep_var[0][1] in s_config:
#                sweep_design_parameter = sweep_var[0][1]
#                sweep_design_component = s_config
#                sweep_parameter_name = s_config['name'] + '_' + sweep_design_parameter
#                sweep_range = sweep_var[1]
#                break;
#
#        if workload.config['name'] == sweep_var[0][0] and sweep_var[0][1] in workload.config:
#           sweep_design_parameter = sweep_var[0][1]
#           sweep_design_component = workload.config
#           sweep_parameter_name = workload.config['name'] + '_' + sweep_design_parameter
#           sweep_range = sweep_var[1]
#
#        for i in sweep_range:
#            print(sweep_parameter_name + ': ' + str(i))
#            sweep_design_component[sweep_design_parameter] = i
#            lifetime, used, possible, missed, online, event_ttc = EHSim.simulate(sweep_config, workload, lights)
#            energy_used = used/possible
#            events_successful = (missed.size - np.sum(missed))/missed.size
#            time_online = np.sum(online) / online.size
#            ttc = event_ttc#np.average(event_ttc)/ workload.config['event_period_s']
#            sweep_result.append([i, lifetime, energy_used, events_successful, time_online, ttc])
#            print("%d%% lifetime" % lifetime)
#            print("%.2f%% Joules used" % (100*energy_used))
#            print("%.2f%% events successful" % (100*events_successful))
#            print("%.2f%% of time online" % (100*time_online))
#            print("%.2f%% x expected event time to completion" % (np.average(event_ttc)/workload.config['event_period_s']))
#
#        sweep_result = np.array(sweep_result, dtype='object')
#        sweep_results.append((sweep_parameter_name, sweep_result))
#
#    for sweep_var, parameter_sweep in zip(sweep.sweep_vars, sweep_results):
#        x = parameter_sweep[1]
#        titlestr = trace_fname.split('/')[-1].split('.')[0] + ' ' + ' '.join(parameter_sweep[0].split('_')[:-1]) + ' ' + ' '.join(workload.config['name'].split('_')) + ' ' + workload_modifier
#        print(titlestr)
#        np.save(save_dir + titlestr.replace(' ', '_'), x)
#        #if sweep_var[2] == 'both' or sweep_var[2] == 'life':
#        #    titlestr = lightfile.split('/')[-1].split('.')[0] + ' lifetime vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
#        #    np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, lifetimes), axis=1))
#        #    if (args.plot):
#        #        # plot lifetimes
#        #        plt.figure()
#        #        plt.grid(True, which='both', ls='-.', alpha=0.5)
#        #        #plt.xscale('log')
#        #        plt.plot(x, lifetimes)
#        #        plt.title(titlestr)
#        #        plt.ylabel('lifetime (years)')
#        #        xlabelstr = ' '.join(parameter_sweep[0].split('_')[1:-1]) + ' (' + parameter_sweep[0].split('_')[-1] + ')'
#        #        plt.xlabel(xlabelstr)
#        #        plt.savefig(save_dir + titlestr.replace(' ', '_'))
#        #if sweep_var[2] == 'both' or sweep_var[2] == 'util':
#        #    titlestr = lightfile.split('/')[-1].split('.')[0] + ' % energy usage vs ' + ' '.join(parameter_sweep[0].split('_')[:-1])
#        #    np.save(save_dir + titlestr.replace(' ', '_'), np.stack((x, percent_used), axis=1))
#        #    if (args.plot):
#        #        # plot % energy utilized
#        #        plt.figure()
#        #        plt.grid(True, which='both', ls='-.', alpha=0.5)
#        #        plt.xscale('log')
#        #        plt.plot(x, percent_used)
#        #        plt.title(titlestr)
#        #        plt.ylabel('% of available harvested energy used')
#        #        plt.xlabel(' '.join(parameter_sweep[0].split('_')[1:]))
#        #        plt.savefig(save_dir + titlestr.replace(' ', '_'))

def run_simulation(a, i, total):
    print('starting simulation {}/{}'.format(i, total))
    # modify config
    location = a[0][a[1]]
    for x in a[2][:-1]:
        location = location[x]
    location[a[2][-1]] = a[3]
    # run simulation with altered
    simulator = EHSim(a[0]['design'], a[0]['workload'], a[0]['dataset'])
    results = simulator.simulate()
    results[a[2][-1]] = a[3]
    return results

def sweep_var(config, var):
    path = config['sweep'][var]['path']
    t = config['sweep'][var]['type']
    print(t, path)
    setup_array = []
    for i, value in enumerate(config['sweep'][var]['values']):
        print('\t' + str(value))
        setup_array.append(([config.copy(), t, path, value], i, len(config['sweep'][var]['values'])))
    #return [run_simulation(x) for x in setup_array]
    with Pool(processes=20) as p:
        print(list(range(0,len(setup_array))))
        print([len(setup_array)]*len(setup_array))
        results = p.starmap(run_simulation, setup_array)
    return results

def sweep(config):
    copy_config = config.copy()
    t = copy_config['sweep']['var1']['type']
    path = copy_config['sweep']['var1']['path']
    results = []

    if 'var2' in copy_config['sweep']:
        for i, value in enumerate(copy_config['sweep']['var1']['values']):
            if copy_config['sweep']['var1']['group']:
                # sweep all variables together
                for x in copy_config['sweep']['var1']['variables']:
                    y = copy_config['sweep']['var1']['variables'][x][i]
                    print(t, x, y)
                    copy_config[t][x] = y
                mapped_result = sweep_var(copy_config, 'var2')
                for result in mapped_result:
                    result[path[-1]] = value
                    results.append(result)
            else:
                # sweep single variable together
                pass

    else:
        results = sweep_var(copy_config, 'var1')

    return results

print('starting sweep')
config = {}
workload = {}
dataset = {}
with open(args.config_file, 'r') as c_file, \
     open(args.workload_file, 'r') as wl_file, \
     open(args.dataset_file, 'r') as d_file, \
     open(args.sweep_file, 'r') as s_file:
    try:
        config = yaml.safe_load(c_file)
        workload = yaml.safe_load(wl_file)['workload']
        dataset = yaml.safe_load(d_file)['dataset']
        sweep_cfg = yaml.safe_load(s_file)['sweep']
    except yaml.YAMLError as e:
        print(e)
        exit(1)

#print(config)
#print(workload)
#print(dataset)
#print(sweep_cfg)

top_config = {'design':config,'workload':workload,'dataset':dataset,'sweep':sweep_cfg}
#sweep_var2(top_config)
results = sweep(top_config)
df = pd.DataFrame(results)
print(df)
df.to_pickle('test.pkl')
