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

for fname in fnames:

    factor = 18.6/50.0 * .93
    #factor = .5
    print(factor)

    data = np.load(fname)
    data[1:] = data[1:]*factor

    #plt.plot(data[1:])

    np.save("light_irradiance" + fname.split("light_lux")[-1], data)
