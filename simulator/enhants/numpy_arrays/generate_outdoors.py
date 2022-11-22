#! /usr/bin/env python3

import numpy as np
from glob import glob

fnames = ['SetupD.npy']
amnts = [10, 50]
for fname in sorted(fnames):
    a = np.load(fname)[1:]
    for amnt in amnts:
        print(a.mean())
        b = a / a.mean() * amnt * 1E-3 * 1E6
        avg = np.average(b)
        print(avg)
        name = fname.split('.')[0] + '_outdoor_{}mW.npy'.format(str(amnt))
        np.save(name, b)
