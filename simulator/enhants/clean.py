#! /usr/bin/env python3

import os
import numpy as np
from glob import glob
from multiprocessing import Pool
import arrow

# Make folders if needed
if not os.path.exists('numpy_arrays'):
    os.makedirs('numpy_arrays')

fnames = glob('enhants_data/*')
def parse(fname):
    basename = fname.split('/')[1].split('_')[0]
    a = np.loadtxt(fname, delimiter='\t', dtype=str)[:-1]
    # strip header
    if(a[0,0].strip() == 'sec'):
        a = a[1:, :]

    # get starting time:
    start_time_string = ''
    for string in a[0][2:]:
        start_time_string += ' ' + string.strip()
    start_time_string = start_time_string.strip()
    start_time = arrow.get(start_time_string, 'YYYY M D H m s')
    b = np.ndarray(a.shape[0] + 1, dtype=float)
    # first element of array is start timestamp
    b[0] = start_time.timestamp

    b[1:] = a[:,1].astype(float)
    # get rid of date/time information
    replace = np.isnan(b)
    b[replace] = 0

    #if len(a) % 2 != 0:
    #    a = np.append(a, 0)
    # average every two datapoints to get to minute granularity
    #data = np.mean(a[:].reshape(-1, 2), axis=1)

    print(fname)
    print('    length: ' + str(b[1:].shape[0]/2/60/24) + ' days')
    print('    average: ' + str(np.mean(b[1:])) + ' uW/cm^2')
    print('    std dev: ' + str(np.std(b[1:])) + ' uW/cm^2')
    np.save('numpy_arrays/' + basename, b)


#p = Pool(len(fnames))
#p.map(parse, sorted(fnames))
for fname in sorted(fnames):
    parse(fname)

