#!/usr/bin/env python3
import os
import logging
#logging.basicConfig(level=logging.DEBUG)
from time import sleep
from keithley2600 import Keithley2600
import numpy as np
import saleae

np.set_printoptions(precision=2)

instrument_serial = 'USB0::1510::9746::4309410\x00::0::INSTR'
dirname = os.path.abspath("traces/")
if not os.path.exists(dirname):
    os.makedirs(dirname)
currents = np.append([0], 1.5*np.power(10.0, np.arange(-6, 0)))
ranges = [100E-6, 100E-6, 10E-3, 10E-3, 10E-3, 10E-3, 100E-3, 1]
voltages = np.arange(2.3, 3.31, .05)
measurements = np.zeros((len(currents), len(voltages), 6))
#print(measurements.shape)
#print(measurements)
#print(currents)
#print(voltages)
#
#for j, _ in enumerate(measurements):
#    print()
#    print(currents[j])
#    for k, _ in enumerate(measurements[j]):
#        print(round(voltages[k], 2))

def avg_ten():
    a_i = 0.
    a_v = 0.
    b_i = 0.
    b_v = 0.
    for i in range(10):
        a_i += k.smua.measure.i()
        a_v += k.smua.measure.v()
        b_i += k.smub.measure.i()
        b_v += k.smub.measure.v()
    return [a_v/10, a_i/10, b_v/10, b_i/10]

s = saleae.Saleae()
s.set_trigger_one_channel(0, saleae.Trigger.Negedge)

k = Keithley2600(instrument_serial)
k.smua.reset()
k.smub.reset()
k.smua.source.output = k.smua.OUTPUT_OFF
k.smub.source.output = k.smub.OUTPUT_OFF
k.smua.sense = k.smua.SENSE_LOCAL
k.smub.sense = k.smub.SENSE_LOCAL
k.smua.measure.nplc = 5
k.smub.measure.nplc = 5

k.smua.measure.autorangei = k.smua.AUTORANGE_ON
k.smua.measure.delay = -1
k.smua.source.func = k.smua.OUTPUT_DCVOLTS
k.smua.source.rangev = 20
#k.smua.measure.rangei = 10E-3
k.smua.source.limiti = 500E-3

k.smub.measure.autorangei = k.smub.AUTORANGE_ON
k.smub.measure.delay = -1
k.smub.source.func = k.smub.OUTPUT_DCAMPS
k.smub.source.limitv = 4
#k.smub.source.rangei = 10E-3
k.smub.source.sink = k.smub.ENABLE

k.smub.source.output = k.smub.OUTPUT_ON
k.smua.source.output = k.smua.OUTPUT_ON

for l, i in enumerate(currents):
    for m, v in enumerate(voltages):
        print('running ' + str(i) + ' Amps, ' + str(round(v, 2)) + ' Volts')
        k.smua.source.levelv = round(v, 2)
        k.smua.measure.lowrangei = ranges[l]
        print(i)
        if l is not 0:
            k.smub.source.leveli = -i
            s.set_capture_seconds(5/(10 ** (l - 1)))
            #print('capture seconds: ' + str(5/(10 ** (l - 1))))
            k.smub.source.output = k.smub.OUTPUT_ON
        else:
            k.smub.source.output = k.smub.OUTPUT_HIGH_Z
            s.set_capture_seconds(10)
        s.capture_start_and_wait_until_finished()
        s.export_data2(dirname + '/' + str(i) + '_' + str(round(v, 2)))
        k_meas = avg_ten()
        # current compensation for saleae measurement
        # saleae has ~2Mohm impedence
        #k_meas[-1] += -k_meas[2] / 2E6
        measurements[l, m, 0] = i
        measurements[l, m, 1] = round(v, 2)
        measurements[l, m, 2:] = avg_ten()
        print(measurements[l, m])
        #k.smua.source.output = k.smua.OUTPUT_OFF
        #k.smub.source.output = k.smub.OUTPUT_HIGH_Z
np.save(dirname + '/measurements.npy', measurements)
print(measurements)
k.smua.source.output = k.smua.OUTPUT_OFF
k.smub.source.output = k.smub.OUTPUT_OFF

#k.smua.reset()
#k.smub.reset()
