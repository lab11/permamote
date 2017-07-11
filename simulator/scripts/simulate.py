#! /usr/bin/env python3

import numpy as np
import arrow
import argparse
import configparser
from itertools import cycle
from datetime import datetime

parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('lightfile', metavar='l', type=str, help='A file to use to simulate light conditions')
parser.add_argument('pirfile', metavar='p', type=str, help='A file to use to simulate wake up events from a PIR sensor')
args = parser.parse_args()

config = configparser.ConfigParser()
config.read('sim_config.ini')
sections = config.sections()

primary_config = config['primary']
secondary_config = config[config['DEFAULT']['secondary']]
#TODO actually use config, add sweep variable


def lux_to_irradiance(lux):
    # irradiance estimation from lux using values reported on page 202 in:
    # http://www.bradcampbell.com/wp-content/uploads/2012/05/yerva-ipsn12-energy_harvesting_leaves.pdf
    #ret = np.ndarray(lux.shape)
    #ind = lux < 75
    #ret[ind] = lux[ind]*18.6/50
    #ind = np.logical_and(lux >= 75, lux < 200)
    #ret[ind] = lux[ind]*29.1/100
    #ind = lux > 200
    #ret[ind] = lux[ind]*74.9/320
    if lux < 75:
        return lux *18.6/50
    elif lux < 200:
        return lux*29.1/100
    else:
        return lux*74.9/320

def get_midnights(times):
    # get unique midnights present in a time series
    return np.unique(times.astype('datetime64[s]').astype('datetime64[D]'))[1:].astype('datetime64[m]')

#def light_sim_step(light):



# load light and motion files
lights = np.load(args.lightfile)
motions = np.load(args.pirfile)

# Find first shared midnight on same day of week
# reduce each data set to midnights
light_data_midnights = get_midnights(lights[:,0])
motion_data_midnights = get_midnights(motions[:,0])

# find larger set
# choose day of week to be first midnight in smaller set
# move forward in larger set until found same day of week
# Want to preserve temporal correlation of light and motion events
larger = "light"
larger_set = light_data_midnights
day_of_week = motion_data_midnights[0].astype(datetime).weekday()
if len(motion_data_midnights) > len(light_data_midnights):
    larger = "motion"
    larger_set = motion_data_midnights
    day_of_week = light_data_midnights[0].astype(datetime).weekday()

larger_start = 0
for midnight in larger_set:
    if midnight.astype(datetime).weekday() == day_of_week:
        larger_start = midnight
        break

if larger == "light":
    ind = lights[:,0].astype('datetime64[s]') >= larger_start
    lights = lights[ind][:,1]
    motions = motions[:,1]
else:
    ind = motions[:,0].astype('datetime64[s]') >= larger_start
    motions = motions[ind][:,1]
    lights = lights[:,1]

i=0
zip_list = zip(lights, cycle(motions)) if larger == "light" else zip(cycle(lights), motions)
for light, motion in zip_list:
    #TODO actually simulate
    i += 1

