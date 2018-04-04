#! /usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
from glob import glob

fnames = glob('*.npy')
lines = []
for fname in sorted(fnames):
    a = np.load(fname)[1:]
    line, = plt.plot(a, label=fname.split('.')[0])
    lines.append(line)
plt.legend(handles = lines)
plt.show()
