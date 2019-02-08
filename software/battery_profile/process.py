#!/usr/bin/env python3
import os
import time
import numpy as np
import argparse
import arrow

parser = argparse.ArgumentParser(description="Process and plot voltage curve for LTO battery")
parser.add_argument('fname', metavar='F', type=str, help="filename of curve data")
args = parser.parse_args()

fname = args.fname
curve = np.load(fname)

now = time.time()
new_curve = []
for x in curve:
    now += x[3]
    new_curve.append([arrow.get(now).format(), x[0], x[1], x[2]])

curve = np.array(new_curve)
print(curve)
np.save(fname, curve)
