#! /usr/bin/env python3

import numpy as np
import matplotlib
#matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
from glob import glob

#minorLocator = MultipleLocator(1/4)
#yLocator = MultipleLocator(10)

fig, ax = plt.subplots(dpi=300)
#plt.ylim(0, 100)
#plt.xlim(0, 10)
plt.grid(True, which='major', ls='-.', alpha=0.5)
#plt.xticks(np.arange(b.size / (60*24)))
#ax.tick_params(axis='x',which='minor')
#ax.xaxis.set_minor_locator(minorLocator)
#ax.yaxis.set_major_locator(yLocator)
plt.xlabel('Time (Days)')
plt.ylabel('Packets per Hour')
a = np.load('seq_no-Ligeiro-c098e5d0004a_clean.npy')[1:]
plt.plot(np.arange(a.size)/24, a, label='experiment')
b = np.load('seq_no-Ligeiro-c098e5d0004a_sim_clean.npy')[1:]
plt.plot(np.arange(b.size)/24, b, label='model')
lgd = plt.legend(loc=0)
plt.show()
plt.savefig('exp_vs_sim_packets_per_hour.pdf',  format='pdf')
