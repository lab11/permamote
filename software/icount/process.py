#!/usr/bin/env python3
import os
import matplotlib.pyplot as plt
import numpy as np
np.set_printoptions(precision=2)
from scipy import stats

dirname = os.path.abspath("traces/") + '/'
meas = np.load(dirname + 'measurements.npy')

data = np.zeros((meas.shape[0], meas.shape[1] + 2))

for j, x in enumerate(meas):
    for k, y in enumerate(x):
        i = y[0]
        v = y[1]
        m = y[2:]
        print('\n' + str(round(v, 2))+ ' - ' + str(i))
        a = np.loadtxt(dirname + str(i) + '_' + str(round(v, 2)) + '.csv', delimiter=',', skiprows = 1)
        print('shape: ' + str(a.shape))
        first = None
        previous = None
        times = []
        for point in a:
            # skip to first 0
            if point[1] != 0 or point[0] < 0:
                continue
            if first is None:
                first = point
                previous = first
                continue
            time_diff = point[0] - previous[0]
            previous = point
            if time_diff < 100E-9:
                continue
            times.append(time_diff)
        # get rid of outliers!
        #print(stats.describe(times))
        std = np.std(times)
        average = np.average(times)
        #print(std)
        times = np.array(times)
        #times_ = times[np.logical_and(times < np.average(times) + std, times > np.average(times) - std)]
        #print(stats.describe(times))
        #average_ = np.average(times_)
        print(average)
        print(1/average)
        #print(average_)
        #print(1/average_)
        data[j] = [i, round(v, 2)] + m + [average, 1/average]
        print(data[j])
        exit()

