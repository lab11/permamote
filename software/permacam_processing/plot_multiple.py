#!/usr/bin/env python3

import matplotlib
#matplotlib.use('Agg')
import matplotlib.pyplot as plt
import argparse
import numpy as np
import pandas as pd
from skimage import img_as_float
from skimage.metrics import structural_similarity as ssim
font = {'family' : 'Arial',
        'weight' : 'medium',
        'size'   : 8}
matplotlib.rc('font', **font)
import os
import shutil
import subprocess
from glob import glob

parser = argparse.ArgumentParser(description='Process quality series of Permacam images and generate mAP measurements compared to raw')
parser.add_argument('save_dir', help='Directory of images/detections')
args = parser.parse_args()

data = {}
numbers = range(1,6)
for n in numbers:
    dirname = args.save_dir + '/' + str(n) + '/'
    fnames = glob(dirname + '*.txt')
    tts = []
    for fname in fnames:
        with open(fname, 'r') as f:
            tts.append(float(f.read()));
    data[n] = tts.copy()
print(data)

fig, ax = plt.subplots(1, figsize=(4,2))
medianprops = dict(linewidth=2)
for n in data:
    d = data[n]
    ax.boxplot(d, positions=[n], widths=0.25, showfliers=True, medianprops=medianprops)
ax.set_ylim(0,100)
ax.set_xlabel('Number of Cameras')
ax.set_ylabel('Time to Send Image (s)')
ax.grid(True, axis='y')
plt.tight_layout()
plt.savefig('multiple_cameras.pdf')
#plt.show()
exit()

