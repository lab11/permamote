#! /usr/bin/env python3

import numpy as np
from scipy import signal
from scipy import stats
import arrow
import math
from itertools import cycle
from datetime import datetime
import matplotlib.pyplot as plt
from opportunistic_config import sim_config

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
    event_energy = (workload_config['radio_energy_J'] + \
            workload_config['sensor_energy_J']) /\
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
    offline = []
    missed = []
    wasted_energy = 0
    possible_energy = 0
    # counter until transmit
    time_to_transmit = workload_config['period_s']
    # wait for secondary to charge
    charge_hysteresis = False;
    for second in seconds:
        primary_discharge = 0
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

        outgoing_energy = sleep_current * design_config['operating_voltage_V']
        if design_config['intermittent'] and design_config['intermittent_mode'] == 'opportunistic':
            available = secondary_energy
            while (available > event_energy):
                outgoing_energy += event_energy
                missed.append((second, 1))
        else:
            # subtract active transmission
            #outgoing_energy = sleep_current * (60 - design_config['active_period_s']) * design_config['operating_voltage_V']
            # double counting sleep current for now
            time_to_transmit -= 1
            if time_to_transmit == 0:
                time_to_transmit = workload_config['period_s']
                if design_config['intermittent'] and design_config['intermittent_mode'] == 'periodic':
                    if secondary_energy < event_energy:
                        missed.append((second, 1))
                    else:
                        missed.append((second, 0))
                        outgoing_energy += event_energy
                else:
                    outgoing_energy += event_energy

        # if not waiting for secondary to recharge, use secondary energy
        if not charge_hysteresis:
            secondary_energy -= outgoing_energy
        # otherwise charge secondary and use primary
        else:
            primary_energy -= outgoing_energy
            primary_discharge += outgoing_energy

        # reset secondary to 0 if negative
        if secondary_energy < 0:
            primary_energy + secondary_energy
            primary_discharge += abs(secondary_energy)
            secondary_energy = 0
            charge_hysteresis = True
        # if secondary low, just charge
        if secondary_energy < secondary_energy_min:
            charge_hysteresis = True
        # if primary is dead, note offline
        if primary_energy <= 0:
            primary_energy = 0
            offline.append(1)
            # if we have a primary cell, break simulation
            if has_primary:
                break;
        else: offline.append(0)

        # subtract leakage
        primary_energy -= primary_leakage_energy
        primary_discharge += primary_leakage_energy
        secondary_energy -= secondary_leakage_energy

        secondary_soc.append(secondary_energy)
        primary_soc.append(primary_energy)

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
    #plt.figure()
    #plt.plot(seconds, [x*1E3/2.4/3600 for x in primary_soc])
    #plt.show()
    #slope, intercept, _, _, _ = stats.linregress(minutes, primary_soc)
    lifetime_years = 15#(primary_energy_max/ np.mean(primary_discharges))/(SECONDS_IN_YEAR)#(intercept/(-slope))/(60*24*365)
    if (lifetime_years > 15): return 15.0, wasted_energy, possible_energy
    #plt.plot([x for x in minutes], [intercept + slope*x*1E3/2.4/3600 for x in minutes])
    #lifetime_years = minute/60/24/365
    return lifetime_years, wasted_energy, possible_energy

config = sim_config()
trace_fname = config.dataset['filename']
trace = np.load(trace_fname)

lifetime, wasted, possible = simulate(config, trace)
print(str(lifetime) + " years")
print("%.2f/%.2f Joules used" % (possible - wasted, possible))

