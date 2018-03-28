#! /usr/bin/env python3

import argparse
import importlib
import numpy as np
from scipy import signal
from scipy import stats
import arrow
import math
from itertools import cycle
from datetime import datetime
import matplotlib.pyplot as plt

SECONDS_IN_YEAR = 60*60*24*365
SECONDS_IN_DAY = 60*60*24
SECONDS_IN_HOUR = 60*60
SECONDS_IN_MINUTE = 60

def simulate(config, lights):
    # load configs
    design_config = config.design_config
    workload_config = config.workload_config
    dataset_config = config.dataset
    if 'secondary' in design_config:
        secondary_config = config.secondary_configs[design_config['secondary']]
    else:
        secondary_config = 0

    # initialize energy for sleep and active
    sleep_current = workload_config['sleep_current_A'] /\
            design_config['boost_efficiency']
    sleep_power = sleep_current * design_config['operating_voltage_V']
    event_energy = workload_config['event_energy_J'] /\
            design_config['boost_efficiency']

    # initialize energy for primary and secondary
    if not design_config['intermittent']:
        has_primary = True
        primary_config = config.primary_config
        if 'volume_L' in primary_config and 'density_Whpl' in primary_config:
            primary_volume = primary_config['volume_L']
            primary_density = primary_config['density_WhpL']
            primary_energy = primary_energy_max = primary_volume * primary_density * SECONDS_IN_HOUR
            primary_leakage_energy = primary_energy * primary_config['leakage_percent_year']/100/SECONDS_IN_YEAR
        else:
            primary_energy = primary_energy_max = primary_config['capacity_mAh'] *\
                    1E-3 * primary_config['nominal_voltage_V'] * SECONDS_IN_HOUR
            primary_leakage_energy = primary_energy * primary_config['leakage_percent_year']/100/SECONDS_IN_YEAR
    else:
        has_primary = False
        primary_energy = 0
        primary_leakage_energy = 0

    solar_config = config.solar_config
    if 'area_cm2' in solar_config:
        solar_area = solar_config['area_cm2']
    else: solar_area = 2 * 100 * primary_volume ** (2.0/3.0)

    if secondary_config:
        # using a battery:
        if secondary_config['type'] == 'battery':
            secondary_energy_max = secondary_config['capacity_mAh']*1E-3*secondary_config['nominal_voltage_V']*SECONDS_IN_HOUR
            secondary_leakage_energy= secondary_config['capacity_mAh'] * 1E-3 / secondary_config['leakage_constant'] * secondary_config['nominal_voltage_V']
            secondary_energy_up = design_config['secondary_max_percent'] / 100 * secondary_energy_max
            secondary_energy_min = design_config['secondary_min_percent'] / 100 * secondary_energy_max
        # using a capacitor:
        elif secondary_config['type'] == 'capacitor':
            secondary_energy_max = secondary_config['capacity_J']
            secondary_leakage_energy = 0
            secondary_energy_up = secondary_config['min_capacity_J']
            secondary_energy_min = secondary_config['min_capacity_J']
        # start at half full
        secondary_energy = (secondary_energy_max + secondary_energy_up) / 2

    # convert trace to second resolution
    seconds = np.arange(lights.size*dataset_config['resolution_s'])

    # begin simulation
    secondary_soc = []
    primary_soc = []
    solar_powers = []
    out_powers = []
    online = [0]
    missed = []
    wasted_energy = 0
    possible_energy = 0
    # counter until transmit
    time_to_transmit = workload_config['period_s']
    # wait for secondary to charge
    charge_hysteresis = False;
    for second in seconds:

        ##
        ## INCOMING ENERGY
        ##
        # energy from solar panel:
        incoming_energy = lights[math.floor(second / dataset_config['resolution_s'])] * 1E-6 * solar_area \
            * solar_config['efficiency'] * design_config['frontend_efficiency']
        #solar_powers.append(incoming_energy)
        possible_energy += incoming_energy

        # charge secondary if possible
        secondary_energy += incoming_energy
        # reset secondary to max if full
        if secondary_energy > secondary_energy_max:
            wasted_energy += secondary_energy - secondary_energy_max
            secondary_energy = secondary_energy_max
        # if charged enough
        if secondary_energy >= secondary_energy_up:
            charge_hysteresis = False

        ###
        ### OUTGOING ENERGY
        ###
        outgoing_energy = 0
        # TODO don't assume events are less than a second
        # amount of time spent in sleep for this workload
        period_sleep = 1
        currently_online = online[-1] == 1
        # if offline, need to pay startup cost
        if not currently_online:
            if secondary_energy - secondary_energy_min > workload_config['startup_energy_J']:
                outgoing_energy += workload_config['startup_energy_J']
                period_sleep -= workload_config['startup_period_s']
                currently_online = 1

        # helper variable for remaining energy in secondary cell
        remaining_secondary_energy = secondary_energy - secondary_energy_min - outgoing_energy
        # we are on or had enough energy to turn on, can we do any work?
        if design_config['intermittent'] and design_config['intermittent_mode'] == 'opportunistic':
            # if there is remaining energy to send
            # TODO support multiple per interation
            if currently_online and remaining_secondary_energy >= event_energy:
                outgoing_energy += event_energy
                period_sleep -= workload_config['event_period_s']
                missed.append(np.array([second, 0]))
        else:
            # if it's time to perform periodic event
            time_to_transmit -= 1
            if time_to_transmit == 0:
                time_to_transmit = workload_config['period_s']
                if not design_config['intermittent']:
                    # if primary cell, can always send
                    missed.append(np.array([second, 0]))
                    outgoing_energy += event_energy
                    period_sleep -= workload_config['event_period_s']
                elif design_config['intermittent_mode'] == 'periodic':
                    if not currently_online or remaining_secondary_energy < event_energy:
                        # don't have enough energy to perform task, sleep until next chance
                        missed.append(np.array([second, 1]))
                    else:
                        # we have energy to perform task
                        outgoing_energy += event_energy
                        period_sleep -= workload_config['event_period_s']
                        missed.append(np.array([second, 0]))

        # update this variable again
        remaining_secondary_energy = secondary_energy - secondary_energy_min - outgoing_energy

        # if we couldn't turn on
        if not currently_online:
            period_on = 0
        # any remaining time is spent sleeping
        # can we sleep the rest of the iteration?
        elif remaining_secondary_energy > sleep_power * period_sleep:
            outgoing_energy += sleep_power * period_sleep
            period_on = 1
        # if not, how long can we sleep for?
        else:
            outgoing_energy += remaining_secondary_energy
            period_on = 1 - period_sleep
            period_on += remaining_secondary_energy / sleep_power
        online.append(period_on)

        ###
        ### UPDATE STATE
        ###
        primary_discharge = 0

        # charge used energy to secondary if possible
        if not charge_hysteresis:
            secondary_energy -= outgoing_energy
        # otherwise charge secondary and use primary
        else:
            primary_energy -= outgoing_energy
            primary_discharge += outgoing_energy

        # reset secondary to minimum if under
        if secondary_energy < secondary_energy_min:
            primary_energy + (secondary_energy - secondary_energy_min)
            primary_discharge += abs(secondary_energy - secondary_energy_min)
            secondary_energy = secondary_energy_min
            charge_hysteresis = True

        # check if now offline
        #is_offline = False
        # if primary and secondary are dead, note offline
        if primary_energy <= 0:
            primary_energy = 0
            # secondary is empty too
            #if charge_hysteresis:
                #is_offline = True
            # if we have a primary cell, break simulation
            if has_primary:
                break;
        #offline.append(is_offline)


        # subtract leakage
        primary_energy -= primary_leakage_energy
        primary_discharge += primary_leakage_energy
        secondary_energy -= secondary_leakage_energy

        secondary_soc.append(secondary_energy)
        primary_soc.append(primary_energy)

    # convert to np array
    missed = np.asarray(missed)

    #print('Averages:')
    ##print('  light ' + str(np.mean(lights)) + ' uW/cm^2')
    #print('  solar ' + str(np.mean(solar_powers)) + ' W')
    #print('  secondary: ' + str(secondary_leakage_energy/60) + ' W')
    #print('  primary: ' + str(primary_leakage_energy/60) + ' W')
    #print('  out ' + str(np.mean(out_powers)) + ' W')
    #print('  total ' + str(np.mean(solar_powers) - primary_leakage_energy/60 - secondary_leakage_energy/60 - np.mean(out_powers)) + ' W')
    #print('  Wasted ' + str(wasted_energy))
    #print('  Possibly Collected ' + str(possible_energy))
    ##print('  minimum primary discharge ' + str(primary_leakage_current))

    #plt.figure()
    #plt.plot(np.arange(lights.size), [x for x in lights])
    #plt.figure()
    #plt.plot(seconds, [x*1E3/2.4/3600 for x in secondary_soc])
    #plt.plot(seconds, [x*1E3/2.4/3600 for x in offline[1:]])
    #plt.plot(missed[:,0], missed[:,1])
    #plt.figure()
    #plt.plot(seconds, [x*1E3/2.4/3600 for x in primary_soc])
    #plt.show()
    #slope, intercept, _, _, _ = stats.linregress(minutes, primary_soc)
    lifetime_years = 15#(primary_energy_max/ np.mean(primary_discharges))/(SECONDS_IN_YEAR)#(intercept/(-slope))/(60*24*365)
    if (lifetime_years > 15): return 15.0, wasted_energy, possible_energy
    #plt.plot([x for x in minutes], [intercept + slope*x*1E3/2.4/3600 for x in minutes])
    #lifetime_years = minute/60/24/365
    online = np.asarray(online)
    return lifetime_years, wasted_energy, possible_energy, missed[:,1], online

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('-c', dest='config', default='permamote_config.py', help='input config file e.g. `permamote_config.py`')
args = parser.parse_args()

imported_config = importlib.import_module(args.config.split('.')[0])
config = imported_config.sim_config()
trace_fname = config.dataset['filename']
trace = np.load(trace_fname)

lifetime, wasted, possible, missed, online = simulate(config, trace)
print(str(lifetime) + " years")
print("%.2f/%.2f Joules used" % (possible - wasted, possible))
print("%.2f%% events successful" % (100 * (missed.size - np.sum(missed))/missed.size))
print("%.2f%% of time online" % (100 * np.sum(online) / online.size))


