#! /usr/bin/env python3

import numpy as np
from scipy import signal
from scipy import stats
import arrow
from itertools import cycle
from datetime import datetime
import matplotlib.pyplot as plt

def simulate(config, lights):
    # load configs
    design_config = config.design_config
    primary_config = config.primary_config
    if 'secondary' in design_config:
        secondary_config = config.secondary_configs[design_config['secondary']]
    else:
        secondary_config = 0

    # initialize energy for sleep and active
    sleep_current = design_config['sleep_current_A'] * \
            design_config['boost_efficiency']
    active_current = design_config['active_current_A'] * \
            design_config['boost_efficiency'] \

    # initialize energy for primary and secondary
    #primary_capacity= primary_config['capacity_mAh']
    primary_volume = primary_config['volume_L']
    primary_density = primary_config['density_WhpL']
    #primary_energy = primary_energy_max = primary_capacity*1E-3*primary_config['nominal_voltage_V']*3600
    primary_energy = primary_energy_max = primary_volume * primary_density * 3600
    #primary_leakage_current = primary_config['leakage_percent_year']/100 * primary_config['capacity_mAh'] /(24*365)/1000
    #primary_leakage_energy = primary_config['nominal_voltage_V'] * primary_leakage_current * 60
    primary_leakage_energy = primary_energy * primary_config['leakage_percent_year']/100 /(60*24*365)

    solar_config = config.solar_config
    if 'area_cm2' in solar_config:
        solar_area = solar_config['area_cm2']
    else: solar_area = 2 * 100 * primary_volume ** (2.0/3.0)
    print(solar_area)
    print(primary_volume)

    # calculate total leakage energy from storage
    secondary_leakage_energy = 0
    secondary_energy = secondary_energy_max = secondary_energy_min = secondary_leakage_energy = 0
    if secondary_config:
        # start at hysterisis
        secondary_capacity = secondary_config['capacity_mAh'] * design_config['secondary_max_percent']
        secondary_energy = secondary_energy_max = secondary_capacity*1E-3*secondary_config['nominal_voltage_V']*3600
        secondary_energy_up = design_config['secondary_max_percent'] * secondary_energy_max
        secondary_energy_min = design_config['secondary_min_percent'] * secondary_capacity * 1E-3*secondary_config['nominal_voltage_V']*3600
        secondary_leakage_current = secondary_config['capacity_mAh'] / 1E3 / secondary_config['leakage_constant']
        secondary_leakage_energy = secondary_config['nominal_voltage_V'] * secondary_leakage_current * 60

    # begin simulation
    secondary_soc = []
    primary_soc = []
    minutes = []
    solar_powers = []
    out_powers = []
    primary_discharges = []
    wasted_energy = 0
    possible_energy = 0
    # counter until transmit
    time_to_transmit = design_config['active_frequency_minutes']
    # wait for secondary to charge
    charge_hysteresis = False;
    for minute, light in enumerate(lights):

        primary_discharge = 0
        # energy from solar panel:
        incoming_energy = light * 1E-6 * solar_area \
                * solar_config['efficiency'] * design_config['frontend_efficiency'] * 60
        solar_powers.append(incoming_energy / 60)
        possible_energy += incoming_energy

        # charge secondary if possible
        secondary_energy += incoming_energy
        # reset secondary to max if full
        if secondary_energy > secondary_energy_max:
            wasted_energy += secondary_energy - secondary_energy_max
            secondary_energy = secondary_energy_max
        # if charged enough
        if secondary_energy > secondary_energy_up:
            charge_hysteresis = False

        # subtract active transmission
        outgoing_energy = sleep_current * (60 - design_config['active_period_s']) * design_config['operating_voltage_V']
        time_to_transmit -= 1
        if time_to_transmit == 0:
            time_to_transmit = design_config['active_frequency_minutes']
            outgoing_energy += active_current * design_config['active_period_s'] * design_config['operating_voltage_V']
        out_powers.append(outgoing_energy/60)

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
        # if primary is dead, break simulation
        if primary_energy <= 0:
            break;
        # subtract leakage
        primary_energy -= primary_leakage_energy
        primary_discharge += primary_leakage_energy
        primary_discharges.append(primary_discharge)
        secondary_energy -= secondary_leakage_energy

        secondary_soc.append(secondary_energy)
        primary_soc.append(primary_energy)
        minutes.append(minute)

    print('Averages:')
    print('  light ' + str(np.mean(lights)) + ' uW/cm^2')
    #print('  solar ' + str(np.mean(solar_powers)) + ' W')
    #print('  secondary: ' + str(secondary_leakage_energy/60) + ' W')
    #print('  primary: ' + str(primary_leakage_energy/60) + ' W')
    #print('  out ' + str(np.mean(out_powers)) + ' W')
    #print('  total ' + str(np.mean(solar_powers) - primary_leakage_energy/60 - secondary_leakage_energy/60 - np.mean(out_powers)) + ' W')
    #print('  Wasted ' + str(wasted_energy))
    #print('  Possibly Collected ' + str(possible_energy))
    #print('  average primary discharge ' + str(np.mean(primary_discharges)/60))
    ##print('  minimum primary discharge ' + str(primary_leakage_current))

    #plt.figure()
    #plt.plot(minutes, [x*1E3/2.4/3600 for x in secondary_soc])
    #plt.show()
    #plt.figure()
    #plt.plot(minutes, [x*1E3/2.4/3600 for x in primary_soc])
    #slope, intercept, _, _, _ = stats.linregress(minutes, primary_soc)
    lifetime_years = (primary_energy_max/ np.mean(primary_discharges))/(60*24*365)#(intercept/(-slope))/(60*24*365)
    if (lifetime_years > 15): return 15.0, wasted_energy, possible_energy
    #plt.plot([x for x in minutes], [intercept + slope*x*1E3/2.4/3600 for x in minutes])
    #lifetime_years = minute/60/24/365
    return lifetime_years, wasted_energy, possible_energy
