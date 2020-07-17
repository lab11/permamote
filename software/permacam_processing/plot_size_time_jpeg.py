#!/usr/bin/env python3
import argparse
import numpy as np
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
matplotlib.rcParams.update({'errorbar.capsize': 3})
font = {'family' : 'Arial',
        'weight' : 'medium',
        'size'   : 8}

matplotlib.rc('font', **font)

parser = argparse.ArgumentParser(description='Process quality series of Permacam images and generate error measurements')
parser.add_argument('save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

print('Loading from file')
manifest_fname = args.save_dir + '/manifest.pkl'
data = pd.read_pickle(manifest_fname)
print(data)
print(np.unique(data['id']).shape)


data['energy'] = data['time'] * 1.108041e-02
quality_group = data.groupby('quality')
print("mean:")
print(quality_group['size', 'time', 'energy'].mean())
print(quality_group['energy'].mean() + 1.70848E-3 + 4.963951E-3)
print(quality_group['time'].mean() + 1.1 + 0.2)
#print("median:")
#print(quality_group['size', 'time', 'energy'].median())
#print("std:")
#print(quality_group['size', 'time', 'energy'].std())
#print("min:")
#print(quality_group['size', 'time', 'energy'].min())
#print("max:")
#print(quality_group['size', 'time', 'energy'].max())


fig, [ax1, ax2, ax3] = plt.subplots(3,figsize=(4,4), sharex=True)
ax1.set_ylabel('Image Size (kB)')
gm = quality_group.mean()
e = quality_group.std()['size']
ax1.errorbar(gm.index, gm['size']/1E3, yerr=e/1E3, ls='none', markersize=3, marker='o', color='tab:blue')
ax1.grid(True)
ax1.set_ylim(0,120)

ax2.set_ylabel('Time to Send (s)')
e = quality_group.std()['time']
ax2.errorbar(gm.index, gm['time'], yerr=e, ls='none', markersize=3, marker='o', color='tab:orange')
ax2.grid(True)
ax2.set_ylim(0,30)

ax3.set_ylabel('Energy (mJ)')
e = quality_group.std()['energy']*1E3
ax3.errorbar(gm.index, gm['energy']*1E3, yerr=e, ls='none', markersize=3, marker='o', color='tab:green')
ax3.set_ylim(0,300)
ax3.grid(True)

plt.setp(ax1.get_xticklabels(), visible=False)
plt.setp(ax2.get_xticklabels(), visible=False)
ax3.set_xlabel("JPEG Quality Factor")
labels = ax3.get_xticks().tolist()
labels = [str(int(x)) for x in labels]
labels[labels.index('100')] = 'raw'
ax3.set_xticklabels(labels)

ax1.tick_params(axis='both', which='major', labelsize=6)
ax2.tick_params(axis='both', which='major', labelsize=6)
ax3.tick_params(axis='both', which='major', labelsize=6)

plt.tight_layout()
fig.subplots_adjust(wspace=0,hspace=0.1)
plt.savefig('size_time_energy.pdf')

