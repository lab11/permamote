#! /usr/bin/env python3
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
import matplotlib.patches as mpatches
from matplotlib.ticker import MaxNLocator
import itertools
import re
from glob import glob

setup_name_to_irradiance = {
    'SetupA': '14.9 μW/cm$^2$',
    'SetupB': '13.8 μW/cm$^2$',
    'SetupD': '64.7 μW/cm$^2$',
    'SetupE': '142 μW/cm$^2$',
}

np.random.seed(42)
cmap = ListedColormap(sns.color_palette('colorblind'))

secondary_size_data = []
reliability_vs_secondary_data = []
fnames = glob('./*.npy')
for fname in fnames:
    # get duty cycle:
    period = fname.split('_')[-1].split('.')[0]

    # get setup name:
    setup_name = setup_name_to_irradiance[fname.split('/')[-1].split('_')[0]]
    to_append = secondary_size_data
    if 'secondary_capacity' in fname:
        to_append = secondary_size_data
    to_append.append([setup_name, period, np.load(fname)])

colors = [x for x in [cmap(0.3), cmap(0.4), cmap(0.9)] for _ in range(0, 3)]
markers = ['o', 's', '^'] * (4)

# plot usage vs secondary:
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
plt.xscale('log')
for i, data in enumerate(sorted(secondary_size_data)):
    plt.plot(data[2][:,0], 100*data[2][:,2], color=colors[i], marker=markers[i], label=data[1] + 's, ' + data[0])
plt.title('Energy Utilized vs Secondary Capacity')
plt.xlabel('Energy Capacity (J)')
plt.ylabel('Energy Utilized (%)')
#handles, labels = plt.gca().get_legend_handles_labels()
#hanbles = sorted(zip(handles, labels), key=lambda x: int(x[1].split('s')[0]))
#handles = [x[0] for x in hanbles]
#labels  = [x[1] for x in hanbles]
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('usage_vs_secondary_size', bbox_extra_artists=(lgd,), bbox_inches='tight')

# plot usage vs secondary:
plt.figure()
plt.grid(True, which='both', ls='-.', alpha=0.5)
plt.xscale('log')
for i, data in enumerate(sorted(secondary_size_data)):
    plt.plot(data[2][:,0], 100*data[2][:,3], color=colors[i], marker=markers[i], label=data[1] + 's, ' + data[0])
plt.title('Reliability vs Secondary Capacity')
plt.xlabel('Energy Capacity (J)')
plt.ylabel('Successful Events (%)')
lgd = plt.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0.)
plt.savefig('events_vs_secondary_size', bbox_extra_artists=(lgd,), bbox_inches='tight')
