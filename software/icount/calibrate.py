#!/usr/bin/env python3
import os
import logging
#logging.basicConfig(level=logging.DEBUG)
from time import sleep
from keithley2600 import Keithley2600
import numpy as np
import saleae

instrument_serial = 'USB0::1510::9746::4309410\x00::0::INSTR'

currents = np.power(10.0, np.arange(-6, 0)) * -1
voltages = np.arange(2.3, 3.31, .05)

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
    return [a_i/10, a_v/10, b_i/10, b_v/10]

s = saleae.Saleae()
s.set_trigger_one_channel(0, saleae.Trigger.Negedge)
s.set_capture_seconds(5)
s.capture_to_file(os.path.abspath('test.csv'))
exit()

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
#k.smua.measure.lowrangei = 10E-3
k.smua.source.func = k.smua.OUTPUT_DCVOLTS
k.smua.source.rangev = 20
#k.smua.measure.rangei = 10E-3
k.smua.source.limiti = 100E-3

k.smub.measure.autorangei = k.smub.AUTORANGE_ON
k.smub.source.func = k.smub.OUTPUT_DCAMPS
k.smub.source.limitv = 4
#k.smub.source.rangei = 10E-3
k.smub.source.sink = k.smub.ENABLE


for i in currents:
    for v in voltages:
        k.smua.source.levelv = v
        k.smub.source.leveli = i
        k.smub.source.output = k.smub.OUTPUT_ON
        k.smua.source.output = k.smua.OUTPUT_ON
        s.capture_to_file('test.csv')
        print(avg_ten())
        k.smua.source.output = k.smua.OUTPUT_OFF
        k.smub.source.output = k.smub.OUTPUT_OFF
        exit()

#k.smua.reset()
#k.smub.reset()
