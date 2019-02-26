#! /usr/bin/env python3
import yaml
import numpy as np
import math
from pprint import pprint
from datetime import datetime
import matplotlib.pyplot as plt

np.random.seed(42)

SECONDS_IN_YEAR = 60*60*24*365
SECONDS_IN_DAY = 60*60*24
SECONDS_IN_HOUR = 60*60
SECONDS_IN_MINUTE = 60

class EHSim:
    def _remaining_secondary_energy(self):
        return self.secondary_energy - self.secondary_energy_min - self.energy_to_turn_on - self.event_energy_min - self.outgoing_sleep_energy

    def _outgoing_energy(self):
                return self.energy_turn_on + self.outgoing_event_energy + self.outgoing_sleep_energy

    def _is_time_to_transmit(self, second):
        # check if period elapsed for periodic workload
        if self.workload_config['type'] == 'periodic':
            if int(second) % int(self.workload_config['period_s']) == 0:
                return 1
            return 0
        # if reactive, use lambda parameter and workload curve
        elif self.workload_config['type'] == 'reactive':
            # get hour of day from second
            remainder = second % SECONDS_IN_DAY
            hour = int(math.floor(remainder / SECONDS_IN_HOUR))
            if hour != _reactive_hour:
                self._reactive_hour = hour
                # get probability of event happening each second in this hour
                p = np.random.poisson(self._lambda_set[hour], 1)
                self._current_hour_P = p / SECONDS_IN_HOUR #np.random.poisson(_lambda_set[hour], 1) / SECONDS_IN_HOUR
            transmit = np.random.random() <= self._current_hour_P
            return transmit
        elif self.workload_config['type'] == 'random':
            return np.random.random() <= (1 / self.workload_config['period_s'])
        else:
            print("Unsupported workload type")
            exit(1)

    # Calculate energy used and if event finished for a given simulation period,
    # where the device has some amount of time and energy available to do work, and
    # remaining event period and energy,
    def _perform_event(self, available_time, available_energy, period_remaining, energy_remaining):
        if energy_remaining <= available_energy:
            # if energy store has enough energy
            if period_remaining <= available_time:
                # and we can do the operation in the available period
                return period_remaining, energy_remaining, 1
            else:
                # we can't, so do some amount of work
                e = available_time * (energy_remaining/period_remaining)
                return available_time, e, 0

        else:
            t = available_energy / (energy_remaining/period_remaining)
            if t <= available_time:
                return t, t * energy_remaining / period_remaining, 0
            else:
                return available_time, available_time * energy_remaining / period_remaining, 0


    def _init_energy(self):
        # initialize energy for sleep and active
        self.sleep_power = self.workload_config['sleep_current_A'] /\
                self.design_config['boost_efficiency']* \
                self.design_config['operating_voltage_V']
        self.event_energy = self.workload_config['event_energy_J'] /\
                self.design_config['boost_efficiency']


        self.event_period_full = self.workload_config['event_period_s']
        self.event_period_min = self.workload_config['event_period_min_s']
        self.energy_turn_on = 0
        if self.workload_config['name'] == 'long_running':
            self.event_energy_min = self.event_energy * self.event_period_min / self.event_period_full
            self.energy_to_turn_on = self.workload_config['startup_energy_J'] +\
            self.event_energy_min
        else:
            self.event_energy_min = self.event_energy
            self.energy_to_turn_on = self.workload_config['startup_energy_J'] +\
                self.event_energy

        self.atomic = self.workload_config['atomic']

        # initialize energy for primary and secondary
        if self.primary_config is not None and self.primary_config['using_primary']:
            self.primary_energy = self.primary_energy_max =\
                    self.primary_config['capacity_mAh']\
                        * 1E-3 * self.primary_config['nominal_voltage_V'] \
                        * SECONDS_IN_HOUR
            self.primary_leakage_power = self.primary_energy *\
                    self.primary_config['leakage_percent_year']\
                        / 100 / SECONDS_IN_YEAR
        else:
            self.primary_energy = 0
            self.primary_energy_max = 0
            self.primary_leakage_power = 0

        if self.secondary_config is not None:
            # using a battery:
            if self.secondary_config['type'] == 'battery':
                self.secondary_energy_max = \
                        self.secondary_config['capacity_mAh'] * 1E-3 *\
                        self.secondary_config['nominal_voltage_V'] * SECONDS_IN_HOUR
                if 'leakage_power_W' in self.secondary_config:
                    # second level simulation, so multiply by 1
                    self.secondary_leakage_power = self.secondary_config['leakage_power_W'] * 1
                else:
                    self.secondary_leakage_power = self.secondary_config['capacity_mAh'] *\
                    1E-3 / self.secondary_config['leakage_constant'] *\
                    self.secondary_config['nominal_voltage_V']
                self.secondary_energy_up = self.secondary_config['secondary_max_percent'] \
                        / 100 * self.secondary_energy_max
                self.secondary_energy_min = self.secondary_config['secondary_min_percent'] \
                        / 100 * self.secondary_energy_max
                assert(self.secondary_energy_max >= self.secondary_energy_up)
                assert(self.secondary_energy_up >= self.secondary_energy_min)
            # using a capacitor:
            elif self.secondary_config['type'] == 'capacitor':
                self.secondary_energy_max = self.secondary_config['capacity_J']
                self.secondary_leakage_power = self.secondary_config['leakage_power_W'] * 1
                self.secondary_energy_up = self.secondary_energy_min = self.secondary_config['min_capacity_J']

            # either start at defined percent, or start bottom hysteresis
            if 'secondary_start_percent' in self.secondary_config:
                self.secondary_energy = self.secondary_energy_max * \
                    self.secondary_config['secondary_start_percent']/100
            else:
                self.secondary_energy = self.secondary_energy_min
        # secondary_config is None
        else:
            self.secondary_leakage_power = self.secondary_energy = \
                    self.secondary_energy_max = self.secondary_energy_up = \
                    self.secondary_energy_min = 0

        # ESR loss estimation
        event_current = self.event_energy / self.event_period_full / self.design_config['operating_voltage_V']
        esr_power = event_current**2 * self.secondary_config['esr_ohm']
        self.event_energy += esr_power * self.event_period_full
        # if we couldn't possibly ever perform any work throw an error
        if (self.atomic and (self.event_energy > self.primary_energy_max + self.secondary_energy_max - self.secondary_energy_min)\
                or self.event_period_min > 1)\
                or self.primary_energy_max + self.secondary_energy_max <= self.workload_config['startup_energy_J']:
            #print(self.atomic and self.event_energy > self.secondary_energy_max - self.secondary_energy_min)
            #print(self.event_energy, self.secondary_energy_max - self.secondary_energy_min)
            #print(self.event_period_min > 1)
            #print(self.secondary_energy_max <= self.workload_config['startup_energy_J'])
            print('ERROR: event either takes too long or takes too much energy with this config to be performed in one step')
            exit(1)

    def simulate(self):
        currently_performing = False
        self.used_energy = self.secondary_energy_min - self.secondary_energy     # keep track of amount of energy used by application
        possible_energy = 0                                                 # amount of possible harvestable energy
        actual_period = 0                                                   # the actual amount of time it took to perform an event

        for second in self.seconds:
            if (second % SECONDS_IN_DAY == 0):
                print('simulating day ' + str(second/SECONDS_IN_DAY))

            ##
            ## INCOMING ENERGY
            ##
            incoming_energy = self.energy_trace[math.floor(second/self.dataset_config['resolution_s'])]
            possible_energy += incoming_energy

            # charge secondary if possible
            self.secondary_energy += incoming_energy

            # reset secondary to max if full
            if self.secondary_energy > self.secondary_energy_max:
                #wasted_energy += self.secondary_energy - self.secondary_energy_max
                self.secondary_energy = self.secondary_energy_max
            # if charged enough, exit charging hysteresis
            if self.charge_hysteresis and self.secondary_energy >= self.secondary_energy_up:
                self.charge_hysteresis = False

            ###
            ### OUTGOING ENERGY
            ###
            period_sleep = 1
            self.currently_online = self.online[-1] == 1

            # if offline, need to pay startup cost
            # don't go online until we can do useful work
            if not self.currently_online and not self.charge_hysteresis:
                if self._remaining_secondary_energy() + self.primary_energy >=\
                self.energy_turn_on:
                    period_sleep -= self.workload_config['startup_period_s']
                    self.currently_online = 1

            # if it's time to perform an event
            # if opportunistic, try to send
            if self.design_config['operating_mode'] == 'opportunistic':
                if self.currently_online:
                    actual_period = 0
                    currently_performing = 1
                    self.event_energy_remaining = self.event_energy
                    self.event_period_remaining = self.event_period_full
                else:
                    self.missed_events.append(np.array([self.trace_start_time+second, 1]))
            elif self._is_time_to_transmit(second + self.trace_start_seconds):
                # we're already working on an event, or we're not online to do
                # event, count it as a miss and try next time
                if currently_performing or not self.currently_online:
                    self.missed_events.append(np.array([self.trace_start_time+second, 1]))
                # reset event energy/period state and start new event
                else:
                    actual_period = 0
                    currently_performing = 1
                    self.event_energy_remaining = self.event_energy
                    self.event_period_remaining = self.event_period_full

            # if we need to work on an event
            if currently_performing and self.currently_online:
                # calculated expected period, energy, and if finished for this cycle
                if not self.design_config['using_primary']:
                    used_p, used_e, finished = self._perform_event(period_sleep, self._remaining_secondary_energy(), self.event_period_remaining, self.event_energy_remaining)
                else:
                    used_p, used_e, finished = self._perform_event(period_sleep, self._remaining_secondary_energy() + self.primary_energy, self.event_period_remaining, self.event_energy_remaining)
                self.outgoing_event_energy = used_e
                self.event_energy_remaining -= used_e
                self.event_period_remaining -= used_p
                period_sleep -= used_p
                # if we finished the event this iteration
                if finished:
                    #events_completed += 1
                    actual_period += used_p
                    #reset event state variables
                    currently_perfoming = 0
                    # successfully completed event
                    self.missed_events.append(np.array([self.trace_start_time+second, 0]))
                    #events.append(start_time+second)
                    self.event_ttc.append(actual_period)
                # if atomic, count not finishing as failure
                elif self.atomic:
                    currently_perfoming = 0
                    self.missed_events.append(np.array([self.trace_start_time+second, 1]))
            actual_period += 1

            #print(remaining_secondary_energy())
            ###
            ### UPDATE STATE
            ###

            if not self.design_config['using_primary']:
                # if we couldn't turn on
                if not self.currently_online:
                    period_on = 0
                # any remaining time is spent sleeping
                # can we sleep the rest of the iteration?
                elif self._remaining_secondary_energy() > self.sleep_power * period_sleep:
                    self.outgoing_sleep_energy = self.sleep_power * period_sleep
                    period_on = 1
                # if not, how long can we sleep for?
                else:
                    self.outgoing_sleep_energy  = self._remaining_secondary_energy()
                    period_on = 1 - period_sleep
                    period_on += self._remaining_secondary_energy() / self.sleep_power
                    self.currently_online = 0
                self.online.append(period_on)

                self.secondary_energy -= self._outgoing_energy()
                self.used_energy += self._outgoing_energy()

                if self.secondary_energy <= self.secondary_energy_min:
                    self.charge_hysteresis = True
            else:
                self.online.append(1)

                if self.charge_hysteresis:
                    self.primary_energy -= self._outgoing_energy()
                else:
                    self.secondary_energy -= self._outgoing_energy()
                    self.used_energy += self._outgoing_energy()
                    # enter hysteresis if under
                    if self.secondary_energy <= self.secondary_energy_min:
                        if self.secondary_energy < 0:
                            self.used_energy += self.secondary_energy
                            # subtract remainder from primary energy
                            self.primary_energy += self.secondary_energy - self.secondary_energy_min
                            secondary_energy = 0
                        #secondary_energy = secondary_energy_min
                        self.charge_hysteresis = True

                # check if now offline
                # if primary and secondary are dead, note offline
                if primary_energy <= 0:
                    break;

            # subtract leakage
            self.primary_energy -= self.primary_leakage_power
            #self.primary_discharge += self.primary_leakage_power
            self.secondary_energy -= self.secondary_leakage_power

            self.secondary_soc.append(self.secondary_energy)
            self.primary_soc.append(self.primary_energy)
        #TODO continue from here:
        # convert to np array
        self.missed_events = np.asarray(self.missed_events, dtype='object')
        self.event_ttc = np.asarray(self.event_ttc)
        #print('Averages:')
        ##print('  light ' + str(np.mean(lights)) + ' uW/cm^2')
        #print('  solar ' + str(np.mean(solar_powers)) + ' W')
        #print('  secondary: ' + str(secondary_leakage_power/60) + ' W')
        #print('  primary: ' + str(primary_leakage_power/60) + ' W')
        #print('  out ' + str(np.mean(out_powers)) + ' W')
        #print('  total ' + str(np.mean(solar_powers) - primary_leakage_power/60 - secondary_leakage_power/60 - np.mean(out_powers)) + ' W')
        #print('  Wasted ' + str(wasted_energy))
        #print('  Possibly Collected ' + str(possible_energy))
        ##print('  minimum primary discharge ' + str(primary_leakage_current))

        #plt.figure()
        #plt.plot(np.arange(lights.size), [x for x in lights])
        #print(len(events))
        #plt.figure()
        #plt.plot(seconds, [x for x in secondary_soc])
        #plt.show()
        #plt.plot(seconds, [x*1E3/2.4/3600 for x in offline[1:]])
        #plt.plot(missed[:,0], missed[:,1])
        #plt.figure()
        #plt.plot(seconds, [x*1E3/2.4/3600 for x in primary_soc])
        #plt.show()
        #slope, intercept, _, _, _ = stats.linregress(minutes, primary_soc)
        if self.primary_config['using_primary']:
            #lifetime_years = (primary_energy_max/ np.mean(primary_discharges))/(SECONDS_IN_YEAR)
            #print (primary_discharge, primary_energy_max)
            lifetime_years = primary_energy_max / (primary_discharge / seconds.size) / SECONDS_IN_YEAR
        else: lifetime_years = -1
        #plt.plot([x for x in minutes], [intercept + slope*x*1E3/2.4/3600 for x in minutes])
        #lifetime_years = minute/60/24/365
        self.online = np.asarray(self.online)

        #np.save('seq_no-Ligeiro-c098e5d00047_sim', events)
        #return lifetime_years, used_energy, possible_energy, self.missed_events[:,1], online, event_ttc

    def __init__(self, platform_config, workload_config, dataset_config):
        # declare class vars
        self.sleep_power = 0            # sleep power of the system

        self.workload_type = ''
        self.event_energy = 0           # total amount of energy required for workload event
        self.event_period_full = 0      # total amount of time required for event
        self.event_period_min = 0       # minimum amount of 'atomic' time required for event
        self.event_energy_min = 0       # minimum amount of energy
        # state for reactive workload
        self.workload_curve = []
        self._lambda_set = None
        self._reactive_hour = None
        self._current_hour_P = None

        self.primary_energy = 0         # current amount of energy stored in primary
        self.primary_energy_max = 0     # full energy capacity of primary
        self.primary_leakage_power = 0  # leakage power of primary cell

        self.secondary_energy     = 0   # current amount of energy stored in secondary
        self.secondary_energy_max = 0   # full energy capacity of secondary
        self.secondary_energy_min = 0   # min hysteresis energy capacity
        self.secondary_energy_up  = 0   # upper hysteresis energy capacity
        self.secondary_leakage_power = 0 # secondary leakage power

        self.solar_area = 0             # area of solar panel
        self.solar_efficiency = 0       # efficiency of solar panel

        self.trace = []                 # harvestable energy trace
        self.trace_resolution = 0       # resolution of energy trace
        self.trace_start_seconds = 0    # starting seconds offset from midnight
        self.trace_start_time = 0       # date of trace start
        self.seconds = []               # array of seconds, trace length

        # simulation generated state
        self.used_energy = 0
        self.secondary_soc = []
        self.primary_soc = []
        self.online = [0]
        self.missed_events = []
        self.event_ttc = []
        self.currently_online = 0
        self.charge_hysteresis = False;
        self.event_period_remaining = 0
        self.event_energy_remaining = 0
        self.outgoing_event_energy = 0
        self.outgoing_sleep_energy = 0


        # set config vars
        self.platform_config = platform_config
        self.workload_config = workload_config
        self.dataset_config = dataset_config
        self.design_config = platform_config['design_config']
        # load secondary/primary/harvester configs, if they exist
        if self.design_config['using_secondary']:
            self.secondary_config = self.platform_config['secondary_config']
        else:
            self.secondary_config = None
        if self.design_config['using_primary']:
            self.primary_config = self.platform_config['primary_config']
        else:
            self.primary_config = None
        if self.design_config['using_harvester']:
            self.harvester_config = self.platform_config['harvester_config']
        else:
            self.harvester_config = None

        # initialize workload, secondary, and primary energy storage
        self._init_energy()

        self.workload_type = self.workload_config['type']
        # load workload curve if reactive
        if self.workload_config['type'] == 'reactive':
            self._lambda_set = np.ceil(self.workload_config['lambda'] * np.load(self.workload_config['curve'])).astype(int)

        # load energy trace
        trace_fname = self.dataset_config['filename']
        self.trace = np.load(trace_fname)
        self.trace_resolution = self.dataset_config['resolution_s']

        # prepare dataset
        self.trace_start_time = self.trace[0].astype('datetime64[s]')
        self.trace_start_seconds = int((self.trace_start_time - self.trace_start_time.astype('datetime64[D]')) / np.timedelta64(1, 's'))
        self.trace = self.trace[1:]
        self.seconds = np.arange(self.trace.size*self.trace_resolution)

        # initialize harvester values
        if self.harvester_config['type'] != self.dataset_config['type']:
            print("Mismatched trace/harvester config")
            exit(1)
        if self.harvester_config['type'] == 'solar' and self.dataset_config['unit'] == 'uW/cm^2':
            if 'area_cm2' in self.harvester_config:
                self.solar_area = self.harvester_config['area_cm2']
                self.energy_trace = self.trace * 1E-6 * self.solar_area * \
                    self.harvester_config['efficiency'] * \
                    self.design_config['frontend_efficiency']
            else:
                print("Missing solar config(s)")
                exit(1)
        # TODO add more harvester options
        else:
            print("Currently unsupported energy trace")
            exit(1)

if __name__ == "__main__":
    import argparse
    import importlib


    # Input files for simulation
    parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
    parser.add_argument('-c', dest='config_file', default='config.yaml', help='input config file e.g. `config.yaml`')
    parser.add_argument('-w', dest='workload_file', default='workload.yaml', help='input workload config file e.g. `workload.yaml`')
    parser.add_argument('-d', dest='dataset_file', default='dataset.yaml', help='input dataset config file e.g. `dataset.yaml`')
    args = parser.parse_args()

    config = {}
    workload = {}
    dataset = {}
    with open(args.config_file, 'r') as c_file, \
         open(args.workload_file, 'r') as wl_file, \
         open(args.dataset_file, 'r') as d_file:
        try:
            config = yaml.safe_load(c_file)
            workload = yaml.safe_load(wl_file)['workload']
            dataset = yaml.safe_load(d_file)['dataset']
        except yaml.YAMLError as e:
            print(e)
            exit(1)
    print(config)
    print(workload)
    print(dataset)
    simulator = EHSim(config, workload, dataset)
    pprint(vars(simulator))
    simulator.simulate()
    exit(0)

    lifetime, used, possible, missed, online, event_ttc = simulate(config, workload, trace)
    print(str(lifetime) + " years")
    print("%.2f/%.2f Joules used" % (used, possible))
    print("%.2f%% events successful" % (100 * (missed.size - np.sum(missed))/missed.size))
    print("%.2f%% of time online" % (100 * np.sum(online) / online.size))
    #print("%.2f%% x expected event time to completion" % (event_ttc / workload.config['event_period_s']))
    print("%.2f%% x expected event time to completion" % (np.average(event_ttc) / workload.config['event_period_s']))


