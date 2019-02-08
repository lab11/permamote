#!/usr/bin/env python3
import os
import time
import numpy as np
import argparse
import arrow
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description="Process and plot voltage curve for LTO battery")
parser.add_argument('fname', metavar='F', type=str, help="filename of curve data")
args = parser.parse_args()

fname = args.fname
curve = np.load(fname)

plt.figure()
plt.plot(curve[:,2].astype('float'))
plt.plot(curve[:,3].astype('float'))
plt.show()
