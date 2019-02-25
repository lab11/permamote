#! /usr/bin/env python3

import numpy as np
import matplotlib
#matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator, FormatStrFormatter
from glob import glob

minorLocator = MultipleLocator(1/4)
yLocator = MultipleLocator(10)

voltages = [1.6223, 1.7489, 1.8431, 1.9180, 1.9838, 2.0463, 2.0983, 2.1442,
        2.1820, 2.2143, 2.2418, 2.2629, 2.2800, 2.2942, 2.3089, 2.3192, 2.3259,
        2.3446, 2.3650, 2.3891, 2.4145, 2.4428, 2.4723, 2.52570, 2.6516, 2.7]
voltages = np.flip(np.load("battery/measurements_5.0C.npy")[:,2], 0)
v_curve = np.ndarray((len(voltages), 2))
v_curve[:,0] = np.linspace(0,100,v_curve.shape[0])
v_curve[:,1] = voltages
print(v_curve)

def v_to_soc(voltage):
    # find place in v curve:
    ind = np.where(v_curve[:,1] < voltage)[0]
    mini = v_curve[ind[-1]]
    maxi = v_curve[ind[-1] + 1]
    diff = maxi - mini
    slope = diff[0]/diff[1]
    soc = slope * (voltage - mini[1]) + mini[0]
    return soc

fname = "secondary_voltage-Permamote-c098e5110003_clean.npy"
sname = "secondary_voltage-Permamote-c098e5110003_sim.npy"
lname = "light_irradiance-Permamote-c098e5110003_clean.npy"
a = np.load(fname)
a[0] = a[1]
# average by 10 mins:
print(a.shape)
a = np.reshape(a, (int(a.shape[0]/10), 10))
print(a.shape)
a = np.mean(a, 1)
b = np.ndarray(a.shape)
for i, d in enumerate(a):
    b[i] = v_to_soc(d)
fig, ax = plt.subplots(dpi=300)
plt.ylim(0, 100)
plt.xlim(0, 7)
plt.grid(True, which='major', ls='-.', alpha=0.5)
#plt.xticks(np.arange(b.size / (60*24)))
#ax.tick_params(axis='x',which='minor')
ax.xaxis.set_minor_locator(minorLocator)
ax.yaxis.set_major_locator(yLocator)
plt.xlabel('Time (Days)')
plt.ylabel('State of Charge (%)')
plt.plot(np.arange(b.size)/(6*24.0), b, label='experiment')
print(a)
print(b)
c = np.load(sname)
print(c)
plt.plot(np.arange(c.size)/(3600*24), 100*c, label='model')
lgd = plt.legend(loc=0)
#ax.annotate('Voltage droop/return', xy = (1.85,57), xycoords='data', xytext=(1.85, 42), arrowprops=dict(arrowstyle='->'), horizontalalignment='center')
#ax.annotate('Charging voltage inflation', xy = (7.63,87), xycoords='data', xytext=(6, 95), arrowprops=dict(arrowstyle='->'), horizontalalignment='center')
#plt.show()
plt.savefig('exp_vs_sim_soc.pdf',  format='pdf')
