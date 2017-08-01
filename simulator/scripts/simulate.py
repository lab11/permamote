#! /usr/bin/env python3

import numpy as np
from scipy import signal
from scipy import stats
import arrow
from itertools import cycle
from datetime import datetime
import matplotlib.pyplot as plt

def lux_to_irradiance(lux):
    # irradiance in uW/cm^2 estimation from lux using values reported on page
    # 202 in:
    # http://www.bradcampbell.com/wp-content/uploads/2012/05/yerva-ipsn12-energy_harvesting_leaves.pdf
    ret = np.ndarray(lux.shape)
    ind = lux < 75
    ret[ind] = lux[ind]*18.6/50
    ind = np.logical_and(lux >= 75, lux < 200)
    ret[ind] = lux[ind]*29.1/100
    ind = lux >= 200
    ret[ind] = lux[ind]*74.9/320
    return ret

# get unique midnights present in a time series
def get_midnights(times):
    return np.unique(times.astype('datetime64[s]').astype('datetime64[D]'))[1:].astype('datetime64[m]')

def simulate(config, lights, motions):
    # load configs
    design_config = config.design_config
    primary_config = config.primary_config
    if 'secondary' in design_config:
        secondary_config = config.secondary_configs[design_config['secondary']]
    else:
        secondary_config = 0

    solar_config = config.solar_config

    # initialize energy for sleep and active
    sleep_energy = design_config['sleep_current_A'] * design_config['operating_voltage_V'] * 60 / design_config['boost_efficiency']
    active_energy = design_config['active_current_A'] *\
            design_config['operating_voltage_V'] *\
            design_config['payload_size_bytes'] * 8 /\
            (design_config['radio_bandwidth_kbps'] * 1E3) / design_config['boost_efficiency']

    # initialize energy for primary and secondary
    primary_capacity= primary_config['capacity_mAh']
    primary_energy = primary_energy_max = primary_capacity*1E-3*primary_config['nominal_voltage_V']*3600
    primary_leakage_current = primary_config['leakage_percent_month']/100 * primary_config['capacity_mAh'] /720/1000
    primary_leakage_energy = primary_config['nominal_voltage_V'] * primary_leakage_current * 60

    # calculate total leakage energy from storage
    secondary_leakage_energy = 0
    secondary_energy = secondary_energy_max = secondary_energy_min = secondary_leakage_energy = 0
    if secondary_config:
        secondary_capacity = secondary_config['capacity_mAh']
        secondary_energy = secondary_energy_max = secondary_capacity*1E-3*secondary_config['nominal_voltage_V']*3600
        secondary_energy_up = design_config['secondary_max_percent'] * secondary_energy_max
        secondary_energy_min = design_config['secondary_min_percent'] * secondary_capacity * 1E-3*secondary_config['nominal_voltage_V']*3600
        secondary_leakage_current = secondary_config['capacity_mAh'] / 1E3 / secondary_config['leakage_constant']
        secondary_leakage_energy = secondary_config['nominal_voltage_V'] * secondary_leakage_current * 60

    # Find first shared midnight on same day of week
    # reduce each data set to midnights
    light_data_midnights = get_midnights(lights[:,0])
    motion_data_midnights = get_midnights(motions[:,0])

    # Shear off the partial day
    lights = lights[lights[:,0].astype('datetime64[s]')< light_data_midnights[-1]]
    lights = lights[lights[:,0].astype('datetime64[s]') >= light_data_midnights[0]]
    motions = motions[motions[:,0].astype('datetime64[s]') < motion_data_midnights[-1]]
    motions = motions[motions[:,0].astype('datetime64[s]') >= motion_data_midnights[0]]

    # find the larger set out of motion and light data
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
        ind = motions[:,0].astype('datetime64[s]') < \
            motion_data_midnights[0].astype('datetime64[s]') + \
            np.timedelta64(7*int(len(motion_data_midnights)/7), 'D')

        motions = motions[ind]
        motions = motions[:,1].astype(bool)
    else:
        ind = motions[:,0].astype('datetime64[s]') >= larger_start
        motions = motions[ind][:,1].astype(bool)
        ind = lights[:,0].astype('datetime64[s]') < \
            light_data_midnights[0].astype('datetime64[s]') + \
            np.timedelta64(7*int(len(light_data_midnights)/7), 'D')

        lights = lights[ind]
        lights = lights[:,1]

    # convert lux to irradiance

    #print(np.mean(lights))
    lights = lux_to_irradiance(lights)
    #print(np.mean(lights))

    # begin simulation
    zip_list = zip(lights, cycle(motions)) if larger == "light" else zip(cycle(lights), motions)
    secondary_soc = []
    primary_soc = []
    minutes = []
    solar_powers = []
    out_powers = []
    # counter until transmit
    time_to_transmit = design_config['active_frequency_minutes']
    # last motion seen
    last_motion = motions[0]
    # wait for secondary to charge
    charge_hysteresis = False;
    for minute, (light, motion) in enumerate(cycle(zip_list)):
        if minute >= 24*60*40*365:
            return -1

        # energy from solar panel:
        incoming_energy = light * 1E-6 * solar_config['area_cm2'] \
                * solar_config['efficiency'] * design_config['frontend_efficiency'] * 60
        solar_powers.append(incoming_energy/60)

        # subtract active transmission and wakeup events
        outgoing_energy = sleep_energy
        time_to_transmit -= 1
        if time_to_transmit == 0:
            time_to_transmit = design_config['active_frequency_minutes']
            outgoing_energy += active_energy
        if not last_motion:
            outgoing_energy += active_energy
        outgoing_energy = outgoing_energy / design_config['boost_efficiency']
        out_powers.append(outgoing_energy/60)

        # if not waiting for secondary to recharge, use secondary energy
        if not charge_hysteresis:
            secondary_energy += incoming_energy - outgoing_energy
        # otherwise charge secondary and use primary
        else:
            secondary_energy += incoming_energy
            primary_energy -= outgoing_energy

        # reset secondary to max or 0
        if secondary_energy > secondary_energy_max:
            secondary_energy = secondary_energy_max
        # if secondary empty
        if secondary_energy < 0:
            primary_energy + secondary_energy
            secondary_energy = 0
            charge_hysteresis = True
        # if charged enough
        if secondary_energy > secondary_energy_up:
            charge_hysteresis = False
        # if secondary low, just charge
        if secondary_energy < secondary_energy_min:
            charge_hysteresis = True
        # if primary is dead, break simulation
        if primary_energy <= 0:
            break;
        # subtract leakage
        primary_energy -= primary_leakage_energy
        secondary_energy -= secondary_leakage_energy

        secondary_soc.append(secondary_energy)
        primary_soc.append(primary_energy)
        minutes.append(minute)

    print('\nAverages:')
    print('  light ' + str(np.mean(lights)) + ' uW/cm^2')
    print('  solar ' + str(np.mean(solar_powers)) + ' W')
    print('  secondary: ' + str(secondary_leakage_energy/60) + ' W')
    print('  primary: ' + str(primary_leakage_energy/60) + ' W')
    print('  out ' + str(np.mean(out_powers)) + ' W')
    print('  total ' + str(np.mean(solar_powers) - primary_leakage_energy/60 - secondary_leakage_energy/60 - np.mean(out_powers)) + ' W')

    #plt.plot([x/(60*24) for x in minutes], [x*1E3/2.4/3600 for x in secondary_soc])
    #plt.show()
    #plt.plot([x/(60*24) for x in minutes], [x*1E3/2.4/3600 for x in primary_soc])
    #plt.show()
    #lifetime_estimate = stats.linregress(minutes, primary_soc)
    #lifetime_years = (lifetime_estimate.intercept/(-lifetime_estimate.slope))/(60*24*365)
    #print('lifetime: ' + str(lifetime_years))
    lifetime_years = minute/60/24/365
    return lifetime_years
