#! /usr/bin/env python3
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import matplotlib.patches as mpatches
from matplotlib.ticker import MaxNLocator
import itertools
from glob import glob

setup_name_to_irradiance = {
    'SetupB': '13.8 μW/cm^2',
    'SetupD': '64.7 μW/cm^2',
    'SetupE': '142 μW/cm^2',
}

np.random.seed(42)
cmap = ListedColormap(sns.color_palette('colorblind'))

life_vs_volume_data = []
life_vs_secondary_data = []
usage_vs_secondary_data = []
fnames = glob('run_results_*%/*.npy')
for fname in sorted(fnames):
    # get duty cycle:
    duty_cycle = fname.split('_')[2].split('/')[0]
    # get setup name:
    setup_name = setup_name_to_irradiance[fname.split('/')[-1].split('_')[0]]
    to_append = life_vs_volume_data
    if 'lifetime_vs_secondary_capacity' in fname:
        to_append = life_vs_secondary_data
    if 'energy_usage_vs_secondary_capacity' in fname:
        to_append = usage_vs_secondary_data
    to_append.append([setup_name, duty_cycle, np.load(fname)])

colors = [x for x in [cmap(0.3), cmap(0.4), cmap(0.9)] for _ in range(0, 3)]
markers = ['o', 's', '^'] * (4)

# plot life vs volume:
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
for i, data in enumerate(sorted(life_vs_volume_data)):
    mask = data[2][:,1] < 15
    if (mask[0] == False): continue
    first_false = np.argmax(mask == False)
    mask[first_false] = True
    to_plot = data[2][mask]
    plt.plot(1000* to_plot[:,0], to_plot[:,1], color=colors[i], marker=markers[i], label=data[1] + ', ' + data[0])
plt.title('Lifetime vs Mote Volume')
plt.xlabel('Volume (cm^3)')
plt.ylabel('Lifetime (Years)')
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('life_vs_volume', bbox_extra_artists=(lgd,), bbox_inches='tight')

# plot life vs secondary:
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
plt.xscale('log')
for i, data in enumerate(sorted(life_vs_secondary_data)):
    mask = data[2][:,1] < 15
    if (mask[0] == False): continue
    first_false = np.argmax(mask == False)
    mask[first_false] = True
    to_plot = data[2][mask]
    plt.plot(to_plot[:,0], to_plot[:,1], color=colors[i], marker=markers[i], label=data[1] + ', ' + data[0])
plt.title('Lifetime vs Secondary Capacity')
plt.xlabel('Capacity (mAh)')
plt.ylabel('Lifetime (Years)')
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('life_vs_secondary', bbox_extra_artists=(lgd,), bbox_inches='tight')

# plot usage vs secondary:
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
plt.xscale('log')
for i, data in enumerate(sorted(usage_vs_secondary_data)):
    plt.plot(data[2][:,0], data[2][:,1], color=colors[i], marker=markers[i], label=data[1] + ', ' + data[0])
plt.title('Energy Utilized vs Secondary Capacity')
plt.xlabel('Capacity (mAh)')
plt.ylabel('Energy Utilized (%)')
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('usage_vs_secondary', bbox_extra_artists=(lgd,), bbox_inches='tight')

comparison_data = []
comparison_fnames = glob('run_results*/*.npy')
comparison_fnames = sorted(list(set(comparison_fnames) - set(fnames)))
mote_names = []

for fname in comparison_fnames:
    mote_names.append(fname.split('/')[0].split('_')[-1])
    comparison_data.append(np.load(fname))

# plot mote comparison
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
#plt.xscale('log')
for name, data in zip(mote_names, comparison_data):
    plt.plot(1000*data[:,0], data[:,1], label=name)
plt.title('Lifetime vs Mote Volume Comparison')
plt.xlabel('Volume (cm^3)')
plt.ylabel('Lifetime (Years)')
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('mote_comparison', bbox_extra_artists=(lgd,), bbox_inches='tight')
