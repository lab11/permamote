#! /usr/bin/env python3

import os
import sys
import numpy as np
from multiprocessing import Pool

data_dir = 'raw_data/'
out_dir = 'clean_data/'
out_dir = os.path.dirname(out_dir) + '/'
if out_dir:
    os.makedirs(out_dir, exist_ok=True)

def process_motion(fname):
    data =  np.loadtxt(data_dir + fname, dtype = 'bool', delimiter=',', usecols=1, converters = {1:bool})
    times = np.loadtxt(data_dir + fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = (times + np.timedelta64(30, 's')).astype('datetime64[m]')
    return clean_and_save(times, data, fname)


def process_light(fname):
    data = np.loadtxt(data_dir+fname, delimiter=',', usecols=1)
    times = np.loadtxt(data_dir+fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = (times + np.timedelta64(30, 's')).astype('datetime64[m]')
    return clean_and_save(times, data, fname)


def clean_and_save(times, data, fname):
    # split data into multiple spans if data represents multiple different
    # periods seperated by at least a day of no data
    ind = []
    previous_time = times[0]
    for i, time in enumerate(times):
        if time - previous_time >= np.timedelta64(1, 'D'):
            ind.append(i)
        previous_time = time
    time_spans = np.split(times, ind)
    data_spans = np.split(data, ind)

    clean_time_spans = []
    clean_data_spans = []
    for light_data, time_span in zip(data_spans, time_spans):
        # Fill in or average duplicates in uncleaned data
        # if multiple data points represent the same minute, average them
        # if a minute's data point is missing, use the previous minutes value
        # if we aren't looking at a day or more of data, skip it
        if time_span[-1] - time_span[0] < np.timedelta64(3, 'D'): continue
        minutes = np.arange(time_span[0], time_span[-1], dtype='datetime64[m]')
        clean_data = np.ndarray(minutes.shape)
        for i, minute in enumerate(minutes):
            match = light_data[time_span == minute]
            if match.shape[0] > 1:
                clean_data[i] = np.mean(match)
            elif match.shape[0] == 1:
                clean_data[i] = match[0]
            else:
                clean_data[i] = clean_data[i-1]
        clean_time_spans.append(time_spans)
        clean_data_spans.append(clean_data)

        # create output directory if necessary
        np.save(out_dir+fname.split('.')[0] + str(minutes[0].astype('datetime64[D]')) + '_times', time_span)
        np.save(out_dir+fname.split('.')[0] + str(minutes[0].astype('datetime64[D]')) + '_clean', clean_data)
        print(fname)
        print('saved time span between ' + str(minutes[0]) + ' and ' + str(minutes[-1]) + '\n\tor ' + str((minutes[-1] - minutes[0]).astype('timedelta64[D]')))
        return 0

light_files = (x for x in os.listdir(data_dir) if 'light' in x)
motion_files= (x for x in os.listdir(data_dir) if 'motion' in x)

pool = Pool(4)
pool.map(process_light, light_files)
pool.map(process_motion, motion_files)
