#!/usr/bin/env python3
import os
import time
import numpy as np
import argparse
import glob
import arrow
import matplotlib
#matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

fnames = glob.glob('measurements*.npy')#args.fname

plt.figure()
ax = plt.gca()
for fname in fnames:
    print(fname)
    curve = np.load(fname)
    print((curve[-1, 0] - curve[0,0])/3600)
    last = curve[0]
    c_v = [[0,curve[0,2],curve[0,3]]]
    for x in curve[1:]:
        #print(x)
        time_diff = x[0] - last[0]
        Ah = time_diff / 3600 * -x[1]
        c_v.append([c_v[-1][0] + Ah, x[2], x[3]])
        last = x
    print(np.array(c_v[-1]))
    c_v = np.array(c_v)
    plt.plot(c_v[:,0] * 1E3, c_v[:,2], label=fname.split("_")[-1].split(".npy")[0])
    #plt.plot(curve[:,3].astype('float'))
lgd = ax.legend()
plt.grid(True, 'both', 'both')
ml = MultipleLocator(1)
ax.xaxis.set_minor_locator(ml)
ax.axhline(y=2.35, lw=1, color='r')
ax.axhline(y=2.33, lw=1, color='r')
plt.ylabel('Cell Voltage')
plt.xlabel('Capacity Discharged (mAh)')
plt.savefig('plot.pdf', format='pdf')
