#! /usr/bin/env python3

import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from multiprocessing import Pool
from datetime import datetime
import arrow

data_dir = 'raw_data/'
out_dir = 'curves/'
out_dir = os.path.dirname(out_dir) + '/'

if out_dir:
    os.makedirs(out_dir, exist_ok=True)

def decode_to_bool(bytes_to_decode):
    if bytes_to_decode == b'True': return True
    else: return False

def plot_data(title, times, data):
    monthsloc = mdates.MonthLocator()
    daysloc= mdates.DayLocator()
    monthsfmt = mdates.DateFormatter('%B')
    daysfmt   = mdates.DateFormatter('%d')

    fig, ax = plt.subplots()
    ax.set_title(title)
    ax.xaxis.set_major_locator(monthsloc)
    ax.xaxis.set_major_formatter(monthsfmt)
    ax.xaxis.set_minor_locator(daysloc)
    ax.xaxis.set_minor_formatter(daysfmt)
    ax.plot(times, data)

def plot_full_data(fname, utc_diff):
    print(fname)
    data =  np.loadtxt(data_dir + fname, dtype = 'bool', delimiter=',', usecols=1, converters = {1:decode_to_bool})
    times = np.loadtxt(data_dir + fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = times + np.timedelta64(utc_diff, 'h')
    # convert to hour granularity
    times = times.astype('datetime64[h]').astype(datetime)
    unique_times = np.unique(times)
    bins = []
    for hour in unique_times:
        bins.append(np.sum(data[times == hour]))
    bins = np.asarray(bins)
    plot_data(fname, unique_times, bins)
    plt.show()

def generate_average_event_curve(fname, utc_diff, date_range):
    print(fname)
    data =  np.loadtxt(data_dir + fname, dtype = 'bool', delimiter=',', usecols=1, converters = {1:decode_to_bool})
    times = np.loadtxt(data_dir + fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    times = times + np.timedelta64(utc_diff, 'h')
    # convert to hour granularity
    times = times.astype('datetime64[h]').astype(datetime)

    # limit to range we're interested in
    selection = np.logical_and(times >= date_range[0], times < date_range[1])
    times = times[selection]
    data = data[selection]
    # get days of activity
    times_days = times.astype('datetime64[D]').astype('datetime64[h]').astype(datetime)
    days = np.unique(times_days)
    avg_curve = []
    for day in days:
        times_in_day = times[times_days == day]
        unique_times = np.unique(times_in_day)
        bins = []
        for hour in unique_times:
            bins.append(np.sum(data[times == hour]))
        bins = np.asarray(bins)
        avg_curve.append(bins)
    avg_curve = np.asarray(avg_curve)
    avg_curve = np.average(avg_curve, axis = 0)
    plot_data(fname, np.arange(bins.size), avg_curve)



motion_files= sorted([x for x in os.listdir(data_dir) if 'motion' in x])
print(motion_files, -8)
#plot_full_data(motion_files[2])
generate_average_event_curve(motion_files[0], -5, (datetime(2016, 7, 30), datetime(2016, 8, 6)))
generate_average_event_curve(motion_files[1], -5, (datetime(2016, 7, 30), datetime(2016, 8, 6)))
generate_average_event_curve(motion_files[2], -8, (datetime(2017, 2, 18), datetime(2017, 2, 25)))
plt.show()
