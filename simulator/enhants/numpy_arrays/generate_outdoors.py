#! /usr/bin/env python3

import matplotlib
matplotlib.use('TKAgg')
import matplotlib.pyplot as plt
import numpy as np
from glob import glob

fnames = ['SetupD.npy']
for fname in sorted(fnames):
    a = np.load(fname)[1:]
    print(a.mean())
    a = a / a.mean() * 16.7
    avg = np.average(a)
    print(avg)
    name = fname.split('.')[0] + '_outdoor.npy'
    np.save(name, a)
