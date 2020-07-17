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

parser = argparse.ArgumentParser(description='Process quality series of Permacam images and generate accuracy measurements')
parser.add_argument('save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

manifest_fname = args.save_dir + '/manifest.pkl'

data = pd.read_pickle(manifest_fname)
ids = np.unique(data['id'])
print(ids)
print(ids.shape)
qualities = np.unique(data['quality'])
print(qualities)

all_arrays = np.zeros((len(qualities)-1, len(ids)))

for x, i in enumerate(sorted(ids)):
    raw_fname = data[(data['id'] == i) & (data['quality'] == 100)]['image_array'].values[0]
    raw_image_array = np.load(raw_fname)
    for y, q in enumerate(sorted(qualities[qualities < 100])):
        jpeg_fname = data[(data['id'] == i) & (data['quality'] == q)]['image_array'].values[0]
        jpeg_image_array = np.load(jpeg_fname)

        all_arrays[y,x] = ssim(raw_image_array, jpeg_image_array, multichannel=True, data_range=raw_image_array.max() - raw_image_array.min())
        #diff = raw_image_array - jpeg_image_array
        #accuracy = 100 * (1 - np.divide(np.abs(diff), 1 + np.abs(raw_image_array)))
        #if q not in pixel_accuracys:
        #    pixel_accuracys[q] = []
        #pixel_accuracys[q].append([np.mean(accuracy, (0,1)), np.var(accuracy, (0,1))])
        #all_arrays[y,x] = np.reshape(accuracy, (320*320, 3))
print(all_arrays)
print(all_arrays.shape)

fig, ax = plt.subplots(1, figsize=(4,2))
ind = np.arange(len(qualities)-1)
width = 0.25
#medianprops = dict(linewidth=0.5)
medianprops = dict(linewidth=2)
whiskerprops = dict(linewidth=0.5)
capprops = dict(linewidth=0.5)
boxprops = dict(linewidth=0.5)
for y, q in enumerate(qualities[qualities < 100]):
    bplot = ax.boxplot(all_arrays[y,:], positions=[y], widths=0.25, showfliers=False, manage_ticks=False, boxprops=boxprops, whiskerprops=whiskerprops, capprops=capprops, medianprops=medianprops)

ax.set_xticks(ind)
ax.set_xticklabels([str(x) for x in qualities[qualities < 100]])
ax.set_ylim(0.6, 1)
ax.set_ylabel('SSIM')
ax.set_xlabel('JPEG quality factor')
ax.grid(True, which='both',axis='y')
plt.tight_layout()
plt.savefig('jpeg_ssim.pdf')
