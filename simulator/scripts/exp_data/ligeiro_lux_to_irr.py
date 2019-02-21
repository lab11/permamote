#! /usr/bin/env python3
import os
import sys
import glob
import numpy as np
import matplotlib.pyplot as plt
from multiprocessing import Pool
from datetime import datetime
import arrow

fnames = glob.glob("light_lux*.npy")

led_factor = 18.6/50
sunlight_factor = led_factor * 1.75#0.35
ligeiro_scaling = 1.0

for fname in fnames:
    print(fname)
    factor = 18.6/200.0

    print(factor)
    print(sunlight_factor/ligeiro_scaling)
    print(led_factor/ligeiro_scaling)
    data = np.load(fname)
    plt.plot(data[1:] * factor, label='old')
    irradiance = data[1:]
    print(np.mean(irradiance))
    irradiance[irradiance < 300] *= led_factor / ligeiro_scaling
    irradiance[irradiance >= 300] *= sunlight_factor / ligeiro_scaling
    print(np.mean(irradiance))
    #plt.plot(data[1:])
    print(np.mean(data[1:] * factor))
    plt.plot(irradiance, label='new')
    #plt.plot(data[1:])
    lgd = plt.legend()
    plt.show()

    data[1:] = irradiance
    np.save("light_irradiance" + fname.split("light_lux")[-1], data)
