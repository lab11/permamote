#!/usr/bin/env python3
import os
import time
from keithley2600 import Keithley2600
import numpy as np
import argparse
import arrow

parser = argparse.ArgumentParser(description="Generate voltage curve for LTO battery")
parser.add_argument('capacity', metavar='C', type=str, help="Battery capacity, in Ah")
parser.add_argument('current', metavar='I', type=float, help="Current, in amps")
args = parser.parse_args()

instrument_serial = 'USB0::1510::9746::4309410\x00::0::INSTR'
battery_capacity_Ah = float(args.capacity)
upper_voltage = 2.7
lower_voltage = 1.4
discharge_current = args.current
measurements = []

k = Keithley2600(instrument_serial)
k.smua.reset()
k.smua.sense = k.smua.SENSE_REMOTE
k.smua.source.func = k.smua.OUTPUT_DCVOLTS
k.smua.measure.autorangei = k.smua.AUTORANGE_ON
k.smua.measure.nplc = 5
k.smua.source.rangev = 20
k.smua.source.output = k.smua.OUTPUT_OFF
k.smua.source.limiti = 100E-3
k.smua.source.levelv = 2.7

k.smua.source.output = k.smua.OUTPUT_HIGH_Z
input("Connect battery to source meter and press a key to start...")
k.smua.source.output = k.smua.OUTPUT_ON

current = k.smua.measure.i()
voltage = k.smua.measure.v()
i = 0
while current > battery_capacity_Ah/10:
    i+=1
    if i % 100 == 0:
        print(current)
        print(voltage)
    current = k.smua.measure.i()
    voltage = k.smua.measure.v()

print("Beginning Test!")

k.smua.source.output = k.smua.OUTPUT_HIGH_Z
k.smua.source.func = k.smua.OUTPUT_DCAMPS
k.smua.measure.autorangev = k.smua.AUTORANGE_ON
k.smua.source.sink = k.smua.ENABLE
k.smua.source.leveli = -discharge_current
k.smua.source.output = k.smua.OUTPUT_ON
disconnect_voltage = voltage
while disconnect_voltage > lower_voltage:
    #get OC voltage
    k.smua.source.leveli = 0
    disconnect_voltage = k.smua.measure.v()
    k.smua.source.leveli = -discharge_current
    #get connected v/c
    current = k.smua.measure.i()
    voltage = k.smua.measure.v()

    measurements.append([time.time(), current, voltage, disconnect_voltage])
    print(measurements[-1])
    time.sleep(30)

k.smua.source.output = k.smua.OUTPUT_HIGH_Z
np.save('measurements_' + str(round(float(discharge_current) / battery_capacity_Ah, 2)) + 'C.npy', np.array(measurements))
