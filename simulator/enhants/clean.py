#! /usr/bin/env python3

import os
import math
import numpy as np
from glob import glob
from multiprocessing import Pool
import arrow

HALF_MINUTES_IN_WEEK = 7*24*60*2
HALF_MINUTES_IN_DAY = 24*60*2

# Make folders if needed
if not os.path.exists('numpy_arrays'):
    os.makedirs('numpy_arrays')

fnames = glob('enhants_data/*.txt')
def parse(fname):
    print(fname)
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

    # get rid of date/time information
    c = a[:,1].astype(float)
    # fill in bad data
    # first get a representative week of good data
    # find transition points
    transitions = np.argwhere(np.diff(~np.isnan(c)).squeeze())
    starts = []
    stops = []
    for idx in transitions:
        if np.isnan(c[idx[0]+1]):
            starts.append(idx[0]+1)
        else:
            stops.append(idx[0]+1)

    for start, stop in zip(starts, stops):
        span_len = stop - start
        replace_size = int(HALF_MINUTES_IN_WEEK * math.ceil(span_len / HALF_MINUTES_IN_WEEK))
        replace_start = start - replace_size
        if replace_start < 0:
            replacement = np.tile(c[0:start], math.ceil(span_len / start))
            c[start:stop] = replacement[:span_len]
        else:
            c[start:stop] = c[replace_start:(replace_start + span_len)]
    b[1:] = c

    #if len(a) % 2 != 0:
    #    a = np.append(a, 0)
    # average every two datapoints to get to minute granularity
    #data = np.mean(a[:].reshape(-1, 2), axis=1)

    print(fname)
    print('    length: ' + str(b[1:].shape[0]/2/60/24) + ' days')
    print('    average: ' + str(np.mean(b[1:])) + ' uW/cm^2')
    # calculate daily percentiles
    #shear off end:
    c = b[1:]
    c = c[:HALF_MINUTES_IN_DAY*math.floor(c.size/HALF_MINUTES_IN_DAY)]
    # bin by days:
    days = []
    for i in range(1, int(c.size/HALF_MINUTES_IN_DAY)):
        days.append(np.average(c[(i-1)*HALF_MINUTES_IN_DAY:i*HALF_MINUTES_IN_DAY]))
    print('    90th percentile: ' + str(np.percentile(days, 90)) + ' uW/cm^2')
    print('    10th percentile: ' + str(np.percentile(days, 10)) + ' uW/cm^2')
    np.save('numpy_arrays/' + basename, b)


#p = Pool(len(fnames))
#p.map(parse, sorted(fnames))
for fname in sorted(fnames):
    parse(fname)

