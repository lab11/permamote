#! /usr/bin/env python3

import numpy as np
import arrow
import argparse
import configparser
from itertools import cycle
from datetime import datetime
import config

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('lightfile', metavar='l', type=str, help='A file to use to simulate light conditions')
parser.add_argument('pirfile', metavar='p', type=str, help='A file to use to simulate wake up events from a PIR sensor')
args = parser.parse_args()

design_config = config.design_config
primary_config = config.primary_config
secondary_config = config.secondary_configs[design_config['secondary']]
solar_config = config.solar_config

# initialize energy for sleep and active
sleep_energy = design_config['sleep_current_A'] * design_config['operating_voltage_V']

# initialize energy for primary and secondary
primary_soc = primary_config['capacity_mAh']
primary_energy = primary_energy_max = primary_soc*1E-3*primary_config['nominal_voltage_V']*3600
primary_leakage_current = primary_config['leakage_percent_month']/100 * primary_config['capacity_mAh'] /720/1000
primary_leakage_energy = primary_config['nominal_voltage_V'] * primary_leakage_current

if secondary_config:
    secondary_soc = secondary_config['capacity_mAh']
    secondary_energy = secondary_energy_max = float(secondary_soc)*1E-3*float(secondary_config['nominal_voltage_V'])*3600
    secondary_leakage_current = secondary_config['leakage_percent_month']/100 * secondary_config['capacity_mAh'] /720/1000
    secondary_leakage_energy = secondary_config['nominal_voltage_V'] * secondary_leakage_current * 60
else:
    secondary_leakage_energy = 0

leakage_energy = primary_leakage_energy + secondary_leakage_energy

def lux_to_irradiance(lux):
    # irradiance in uW/cm^2 estimation from lux using values reported on page
    # 202 in:
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

# load light and motion files
lights = np.load(args.lightfile)
motions = np.load(args.pirfile)

# Find first shared midnight on same day of week
# reduce each data set to midnights
light_data_midnights = get_midnights(lights[:,0])
motion_data_midnights = get_midnights(motions[:,0])

# Shear off the last partial day
lights = lights[lights[:,0].astype('datetime64[s]')< light_data_midnights[-1]]
motions = motions[motions[:,0].astype('datetime64[s]') < motion_data_midnights[-1]]

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

time_to_transmit = design_config['active_period_minutes']
zip_list = zip(lights, cycle(motions)) if larger == "light" else zip(cycle(lights), motions)
for light, motion in zip_list:

    incoming_energy = lux_to_irradiance(light) * 1E-6 * solar_config['area_cm2'] \
            * solar_config['efficiency'] * design_config['frontend_efficiency'] * 60

    energy_budget = incoming_energy - sleep_energy - leakage_energy
    print(energy_budget)
    secondary_energy += energy_budget
    if secondary_energy > secondary_energy_max:
        secondary_energy = secondary_energy_max
    if secondary_energy < 0:
        primary_energy + secondary_energy
        secondary_energy = 0
    print(secondary_energy)
    print(primary_energy)
    #time_to_transmit -= 1
    #if time_to_transmit == 0:
    #    active_time = float(design_config['active_time_seconds'])
    #    operating_energy = active_time * float(design_config['active_current_A') \
    #            + (60 - active_time) * float(design_config['sleep_current_A']) \
    #            +
