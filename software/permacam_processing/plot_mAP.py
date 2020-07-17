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

manifest_fname = args.save_dir + '/manifest.pkl'

data = pd.read_pickle(manifest_fname)
ids = np.unique(data['id'])
print(ids.shape[0])
qualities = np.unique(data['quality'])
qualities = sorted(qualities[qualities < 100])

#clean out and copy ground truth to mAP directory
ground_truth_dir = "mAP/input/ground-truth/"
detections_dir = "mAP/input/detection-results/"

# clear out previous files
files = glob(ground_truth_dir + "*") + glob(detections_dir + "*")
for f in files:
    if os.path.isfile(f):
        os.remove(f)
    elif os.path.isdir(f):
        shutil.rmtree(f)

# copy ground truth files
files = glob(args.save_dir + "/detections/100/*")
print(len(files))
for f in files:
    pure_name = f.split('/')[-1]
    shutil.copyfile(f, ground_truth_dir + pure_name)

mAPs = {"quality": qualities, "mAP": []}
for q in qualities:
    # copy detections for qualities into detections dir
    files = glob(args.save_dir + "/detections/" + str(q) + "/*")
    for f in files:
        pure_name = f.split('/')[-1]
        shutil.copyfile(f, detections_dir + pure_name)

    # perform mAP calculation for quality
    subprocess.run(["python3", "scripts/extra/intersect-gt-and-dr.py"], cwd="mAP/", capture_output=True)
    output = subprocess.run(["python3", "main.py", "-np", "-na"], cwd="mAP/", capture_output=True)
    mAP = float(output.stdout.decode('utf-8').split()[-1].split('%')[0])
    print(mAP)
    mAPs["mAP"].append(mAP)

df = pd.DataFrame(mAPs).set_index("quality")
print(df)

fig, ax = plt.subplots(1, figsize=(4,2))

ax.plot(df.index, df['mAP'], marker='.')
ax.set_xticks(qualities)

ax.set_ylabel('mAP')
ax.set_xlabel('JPEG quality factor')
ax.set_ylim(0,100)
ax.grid(True, axis='y')
plt.tight_layout()
plt.savefig('jpeg_mAP.pdf')

