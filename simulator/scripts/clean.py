#! /usr/bin/env python3

import os
import sys
import numpy as np
import arrow

active_current = 10E-3
sleep_current = 5E-6

solar_panel_eff = .19 #%
solar_panel_area = 14 #cm^2


def lux2irradiance(lux):
    # irradiance estimation from lux using values reported on page 202 in:
    # http://www.bradcampbell.com/wp-content/uploads/2012/05/yerva-ipsn12-energy_harvesting_leaves.pdf
    lux[lux < 75] = lux[lux < 75]*18.6/50
    lux[np.logical_and(lux >= 75, lux < 200)] = lux[np.logical_and(lux >= 75, lux < 200)]*29.1/100
    lux[lux > 200] = lux[lux > 200]*74.9/320

data_dir = 'raw_data/'

light_files = (data_dir + x for x in os.listdir(data_dir) if 'light' in x)
motion_files= (data_dir + x for x in os.listdir(data_dir) if 'motion' in x)

fname_to_days = {}
fname_to_data = {}
for fname in light_files:
    print(fname)
    lights = np.loadtxt(fname, delimiter=',', usecols=1)
    light_times = np.loadtxt(fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    days = np.arange(light_times[0], light_times[-1], dtype='datetime64[D]')
    light_days = light_times.astype('datetime64[D]')
    valid_days = []
    for day in days:
        ind = light_days == day
        if np.sum(lights[ind] == 0)/lights[ind].shape[0] < 0.6:
            valid_days.append(day)
    valid_days = np.asarray(valid_days)
    fname_to_days[fname] = valid_days
    fname_to_data[fname] = lights[np.in1d(light_days, valid_days)]
    print(fname_to_data[fname].shape)
    print(lights.shape)
    print(fname_to_data[fname])
    #power_uW = lights[1440*3:1440*4]*solar_panel_eff*solar_panel_area
    #print(np.mean(power_uW/3.3))


for fname in motion_files:
    print(fname)
    motions = np.loadtxt(fname, dtype = 'bool', delimiter=',', usecols=1, converters = {1:bool})
    times = np.loadtxt(fname, dtype = 'datetime64', delimiter=',', usecols=0, converters = {0:np.datetime64})
    days = np.arange(times[0], times[-1], dtype='datetime64[D]')

