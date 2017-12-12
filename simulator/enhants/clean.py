#! /usr/bin/env python3

import os
import numpy as np
from glob import glob
from multiprocessing import Pool

# Make folders if needed
if not os.path.exists('numpy_arrays'):
    os.makedirs('numpy_arrays')

fnames = glob('enhants_data/*')
def parse(fname):
    print('    ' + fname)
    basename = fname.split('/')[1].split('_')[0]
    a = np.loadtxt(fname, delimiter='\t', dtype=str)
    if(a[0,0].strip() == 'sec'):
        a = a[1:, :]
    a = a[:,1]
    a = a.astype(float)
    # get rid of date/time information
    replace = np.isnan(a)
    a[replace] = 0
    if len(a) % 2 != 0:
        a = np.append(a, 0)
    # average every two datapoints to get to minute granularity
    data = np.mean(a[:].reshape(-1, 2), axis=1)
    np.save('numpy_arrays/' + basename, data)


p = Pool(len(fnames))
p.map(parse, sorted(fnames))
#for fname in sorted(fnames):
#    parse(fname)

