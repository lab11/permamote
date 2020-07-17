#!/usr/bin/env python3
import matplotlib as mpl
#mpl.use('Agg')
font= {'family': 'Arial',
        'size': 7}
mpl.rc('font', **font)
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.dates as md
import numpy as np
from pytz import timezone
import pandas as pd
from glob import glob
import arrow
import os
from scipy.signal import find_peaks

fname = "power2/permacam_2.csv"

print(fname)
if not os.path.exists("power2/power.pkl"):
    data = np.genfromtxt(fname, dtype=float, delimiter=',',skip_header=11)
    data = pd.DataFrame({'timestamp':data[:,0], 'D': data[:,1], 'IL_valid': data[:,7], 'IH': data[:,8]*1E-9, 'IL':data[:,9]*10E-12, 'V':data[:,10]*10E-9})
    data.timestamp = data.timestamp.ffill() + data.groupby(data.timestamp.notnull().cumsum()).cumcount()/32E3
    data.timestamp = pd.to_datetime(data.timestamp, unit='s')
    data.timestamp.dt.tz_localize('UTC').dt.tz_convert('US/Pacific')
    data.to_pickle("power2/power.pkl");
else:
    data = pd.read_pickle("power2/power.pkl")

print(data)
data['current'] = data['IH']
#data.loc[np.logical_not(data['IL_valid']), 'current']= data['IH'][np.logical_not(data['IL_valid'])]
print(data)

data = data[int(2.08811E6):]
data['diff'] = data['D'].diff()
posedge = np.where(data['diff'] == 1)[0]
negedge = np.where(data['diff'] == -1)[0]
print(len(posedge))
print(len(negedge))
data['power'] = -2.8 * data['current']

#data = data.set_index('timestamp')
#mv = data.rolling(200).mean()
plt.plot(-data['current'])
plt.plot(data['D'])
#plt.plot(data['diff'])
plt.show()

avg_power = []
energy = []
time = []
for pos, neg in zip(posedge, negedge):
    time.append((neg - pos) * 1/32E3)
    slc = data['power'][pos:neg]
    energy.append(np.trapz(slc, dx = 1/32E3))
    avg_power.append(np.mean(slc))

time = np.array(time).reshape(int(len(time)/3), 3)
energy = np.array(energy).reshape(int(len(energy)/3), 3)
avg_power = np.array(avg_power).reshape(int(len(avg_power)/3), 3)
files = glob('power/*.jpeg')
files.sort(key=os.path.getmtime)
files = files[5:]
file_size = list(map(os.path.getsize, files))
image_data = {}
image_data['files'] = files
image_data['size'] = file_size
image_data['e_capture'] = energy[:,0]
image_data['e_compress'] = energy[:,1]
image_data['e_send'] = energy[:,2]
image_data['p_send'] = avg_power[:,2]
image_data['t_capture'] = time[:,0]
image_data['t_compress'] = time[:,1]
image_data['t_send'] = time[:,2]
image_data = pd.DataFrame(image_data)
image_data['e_per_bit'] = np.divide(image_data['e_send'], (image_data['size'] * 8))
image_data['t_per_bit'] = np.divide(image_data['t_send'], (image_data['size'] * 8))
image_data['goodput'] = 1 / image_data['t_per_bit']
image_data['p_send_avg'] = np.divide(image_data['e_send'], (image_data['t_send']))
mean  = image_data.mean(numeric_only=True)
print(image_data)
print(mean)
image_data.to_pickle("power2/power_summary.pkl")
