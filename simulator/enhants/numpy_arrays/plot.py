#! /usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
from glob import glob

seconds_in_day = 60*60*24

fnames = glob('*.npy')
lines = []
for fname in sorted(fnames):
    a = np.load(fname)[1:]
    print(fname, np.mean(a))
    a = a[:int(int(len(a) / (seconds_in_day / 30)) * (seconds_in_day / 30))]
    a = a.reshape(-1, int(seconds_in_day / 30))
    a = np.mean(a, 1)
    line, = plt.plot(a)
    #line, = plt.plot(a, label=fname.split('.')[0])
    lines.append(line)
    plt.ylabel('µW/cm^2')
    plt.xlabel('day')
    plt.title(fname.split('.')[0])
    plt.savefig(fname.split('.')[0] + '_raw.png', dpi=300)
    plt.close()
