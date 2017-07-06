#! /usr/bin/env python3

import os
import sys
import numpy as np
import arrow

active_current = 10E-3
sleep_current = 5E-6

solar_panel_eff = .19 #%
solar_panel_area = 14 #cm^2

mins_in_day = 60*24

def lux2irradiance(lux):
    # irradiance estimation from lux using values reported on page 202 in:
    # http://www.bradcampbell.com/wp-content/uploads/2012/05/yerva-ipsn12-energy_harvesting_leaves.pdf
    lux[lux < 75] = lux[lux < 75]*18.6/50
    lux[np.logical_and(lux >= 75, lux < 200)] = lux[np.logical_and(lux >= 75, lux < 200)]*29.1/100
    lux[lux > 200] = lux[lux > 200]*74.9/320

data_dir = 'raw_data/'
out_dir = 'clean_data/'

light_files = (data_dir + x for x in os.listdir(data_dir) if 'light' in x)
motion_files= (data_dir + x for x in os.listdir(data_dir) if 'motion' in x)

fname_to_days = {}
fname_to_data = {}
for fname in light_files:
    print(fname)
    data = np.loadtxt(fname, delimiter=',', usecols=1)
    times = np.loadtxt(fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = (times + np.timedelta64(30, 's')).astype('datetime64[m]')

    # split data into multiple spans if data represents multiple different
    # periods seperated by at least a day of no data
    ind = []
    previous_time = times[0]
    for i, time in enumerate(times):
        if time - previous_time >= np.timedelta64(30, 'm'):
            ind.append(i)
        previous_time = time
    spans = np.split(times, ind)
    datas = np.split(data, ind)

    for i, (light_data, time_span) in enumerate(zip(datas, spans)):
        # Fill in or average duplicates in uncleaned data
        # if multiple data points represent the same minute, average them
        # if a minute's data point is missing, use the previous minutes value
        # if we aren't looking at a day or more of data, skip it
        if time_span[-1] - time_span[0] < np.timedelta64(1, 'D'): continue
        minutes = np.arange(time_span[0], time_span[-1], dtype='datetime64[m]')
        cleaned = np.ndarray(minutes.shape)
        for i, minute in enumerate(minutes):
            match = light_data[time_span == minute]
            if match.shape[0] > 1:
                cleaned[i] = np.mean(match)
            elif match.shape[0] == 1:
                cleaned[i] = match[0]
            else:
                cleaned[i] = cleaned[i-1]
        print(time_span)
        print(cleaned)
        # create output directory if necessary
        out_dir = os.path.dirname(out_dir)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        np.save(fname.split('.')[0] + '_times_' + str(i), time_span)
        np.save(fname.split('.')[0] + '_clean_' + str(i), cleaned)

for fname in motion_files:
    print(fname)
    motions = np.loadtxt(fname, dtype = 'bool', delimiter=',', usecols=1, converters = {1:bool})
    times = np.loadtxt(fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = (times + np.timedelta64(30, 's')).astype('datetime64[m]')

