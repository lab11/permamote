#!/usr/bin/env python3

import matplotlib
matplotlib.use('Agg')
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

manifest_fname = args.save_dir + '/distance_data.pkl'

data = pd.read_pickle(manifest_fname)
types = np.unique(data['type'])
cam_data = data[data['type'] == 'cam']
phone_data = data[data['type'] == 'phone']

fig, ax = plt.subplots(1, figsize=(4,2))

ax.plot(cam_data['distance'], cam_data['confidence'], marker='.', label='Eternacam')
ax.plot(phone_data['distance'], phone_data['confidence'], marker='.', label='Pixel 3')
ax.set_xticks(np.unique(data['distance']))

ax.set_ylabel('Detection Confidence (%)')
ax.set_xlabel('Distance to Camera (m)')
ax.set_ylim(0,110)
ax.grid(True, axis='y')
ax.legend()
plt.tight_layout()
plt.savefig('distance_detection.pdf')

