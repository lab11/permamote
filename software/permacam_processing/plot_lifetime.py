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

parser = argparse.ArgumentParser(description='Process simulation data for periodic + event')
parser.add_argument('save_dir', help='Directory for data')
args = parser.parse_args()

print('Loading from file')
periodic_fname = args.save_dir + '/periodic.pkl'
event_fname = args.save_dir + '/event.pkl'
event_60_fname = args.save_dir + '/event_60.pkl'
p_data = pd.read_pickle(periodic_fname)
e_data = pd.read_pickle(event_fname)
e60_data = pd.read_pickle(event_60_fname)
print(p_data)
print(e_data)

# plot periodic

colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'tab:pink', 'tab:gray', 'tab:olive', 'tab:cyan']

fig, ax = plt.subplots(1,figsize=(4,2))
#fig, (ax, ax)= plt.subplots(1,2,figsize=(8,2), sharey=True)
ax.set_xlabel('JPEG Quality Factor')
ax.set_ylabel('Lifetime (years)')
for c, p in zip(colors, np.unique(p_data['period_s'])):
    subset = p_data[p_data['period_s'] == p]
    ax.plot(subset['jpeg_quality'].astype(str), subset['lifetime'], markersize=1, marker='o', color=c, label= str(p)+ " s")
ax.grid(True)
ax.set_ylim(0,10)
#plt.legend(bbox_to_anchor=(1.04,1), loc="upper left")
plt.savefig('lifetime_periodic.pdf' , bbox_inches='tight')

#events = e_data['events_success'] / 320
#print(events)
#print(events/24)
#
#fig, ax = plt.subplots(1,figsize=(4,2))
#ax.set_xlabel('JPEG Quality Factor')
#ax.set_ylabel('Lifetime (years)')
#for c, p in zip(colors, np.unique(e_data['backoff_s'])):
#    subset = e_data[e_data['backoff_s'] == p]
#    ax.plot(subset['jpeg_quality'].astype(str), subset['lifetime'], markersize=1, marker='o', color=c, label= str(p)+ " s")
#ax.grid(True)
#ax.set_ylim(0,10)
#plt.legend(bbox_to_anchor=(1.04,1), loc="upper left")
#plt.tight_layout()
#plt.savefig('lifetime_event.pdf')

fig, ax = plt.subplots(1,figsize=(4,2))
ax.set_xlabel('JPEG Quality Factor')
ax.set_ylabel('Lifetime (years)')
for c, p in zip(colors, np.unique(e60_data['backoff_s'])):
    subset = e60_data[e60_data['backoff_s'] == p]
    ax.plot(subset['jpeg_quality'].astype(str), subset['lifetime'], markersize=1, marker='o', color=c, label= str(p)+ " s")
ax.grid(True)
ax.set_ylim(0,10)
lgd = plt.legend(bbox_to_anchor=(1.04,1), loc="upper left", borderaxespad=0)
plt.savefig('lifetime_event_60.pdf', bbox_extra_artists=(lgd,), bbox_inches='tight')

