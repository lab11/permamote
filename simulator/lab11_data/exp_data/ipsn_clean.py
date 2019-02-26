#! /usr/bin/env python3

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from datetime import timedelta
import arrow
import glob
from multiprocessing import Pool

def clean(times, data):
    minutes = np.arange(times[0], times[-1], dtype='datetime64[m]')
    times_minutes = times.astype('datetime64[m]')
    clean_data = np.ndarray(minutes.shape[0], dtype='float64')
    for i, minute in enumerate(minutes):
        match = data[times_minutes == minute]
        if match.shape[0] > 1:
            clean_data[i] = np.mean(match)
        elif match.shape[0] == 1:
            clean_data[i] = match[0]
        else:
            clean_data[i] = clean_data[i-1]
    # get start time:
    start = arrow.get(str(times[0].astype('datetime64[s]'))).timestamp
    # put it at the front
    a = np.ndarray(clean_data.size + 1)
    a[1:] = clean_data
    a[0] = float(start)
    #plt.figure()
    #plt.plot(a[1:])
    #plt.show()
    #exit()
    #print(' done:' + fname)
    #np.save(out_dir+fname.split('.')[0] +  '_clean', a)
    return a

def Ligeiro_clean(times, data):
    days = times.astype('datetime64[D]')
    hours = np.arange(days[0].astype('datetime64[h]'), days[-1].astype('datetime64[h]'), dtype='datetime64[h]')
    times = times.astype('datetime64[h]')
    clean_data = np.ndarray(hours.shape)
    # reduce resolution to hours
    for i, hour in enumerate(hours):
        match = times == hour
        if(not np.any(match)):
            clean_data[i] = 0
            continue
        first = data[match][0]
        last = data[match][-1]
        clean_data[i] = last - first#np.sum(match)
    start = arrow.get(str(times[0].astype('datetime64[s]'))).timestamp
    a = np.ndarray(clean_data.size + 1)
    a[1:] = clean_data
    a[0] = start
    return a
    #np.save(out_dir+fname.split('.')[0] +  '_clean', a)
    #return 0

def fix_data(fname):
    print('start: ' + fname)
    times = np.loadtxt(fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})[1:]
    data = np.loadtxt(fname, delimiter=',', usecols=1)[1:]
    i = 0

    if np.any(data[i] > 2.5):
        while data[i] < 2.5:
            i+=1
    times = times[i:]
    data = data[i:]

    if('Ligeiro' in fname):
        a = Ligeiro_clean(times, data)
    else:
        a = clean(times, data)
    np.save(fname.split('.')[0] + '_clean.npy', a)

fnames = glob.glob('*csv')

for fname in glob.glob('*Ligeiro*sim.npy'):
    sim_times = np.load(fname)
    sim_data = np.arange(sim_times.shape[0])
    a = Ligeiro_clean(sim_times, sim_data)
    np.save(fname.split('.')[0] + '_clean.npy', a)

for fname in glob.glob('*Permamote*sim.npy'):
    sim_data = np.load(fname)
    print(sim_data)
    base = np.datetime64('2000-01-01 00:00:00')
    times = np.array([base + np.timedelta64(seconds=i) for i in range(sim_data.shape[0])])
    print(times)
    a = clean(times, sim_data)
    np.save(fname.split('.')[0] + '_clean.npy', a)

pool = Pool(10)
pool.map(fix_data, fnames)

