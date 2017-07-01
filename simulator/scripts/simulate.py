#! /usr/bin/env python3

import os
import sys
import numpy as np
from matplotlib.dates import date2num

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

print("light")
for fname in light_files:
    print(fname)
    lights = np.loadtxt(fname, delimiter=',', usecols=1)
    lux2irradiance(lights)
    power_uW = lights[1440*3:1440*4]*solar_panel_eff*solar_panel_area
    print(np.mean(power_uW/3.3))

#motions = []
#print("motion")
#for fname in motion_files:
#    array = np.loadtxt(fname, delimiter=',', converters = {1: bool}, usecols=1)
#    print(fname)
#    print(array.size)


