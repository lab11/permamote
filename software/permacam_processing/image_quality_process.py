#!/usr/bin/env python3

import argparse
from glob import glob
from skimage import exposure
from PIL import Image
import colour
import numpy as np
import pandas as pd
import os
import matplotlib
import matplotlib.pyplot as plt
matplotlib.rcParams.update({'errorbar.capsize': 2})
from imageai.Detection import ObjectDetection

from colour_demosaicing import (
    demosaicing_CFA_Bayer_bilinear,
    demosaicing_CFA_Bayer_Malvar2004,
    demosaicing_CFA_Bayer_Menon2007,
    mosaicing_CFA_Bayer)
cctf_encoding = colour.cctf_encoding

parser = argparse.ArgumentParser(description='Process quality series of Permacam images and generate error measurements')
parser.add_argument('save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

jpegs = glob(args.save_dir + '/image_*.jpeg')
raws = glob(args.save_dir + '/image_raw*.npy')
manifest_fname = args.save_dir + '/manifest.pkl'
detect_fname = args.save_dir + '/detect.pkl'
data = None
if not os.path.isfile(manifest_fname) :
    print('Generating')
    data = {}
    data['filename'] = []
    data['id'] = []
    data['quality'] = []
    data['size'] = []
    data['raw'] = []
    data['time'] = []
    data['image_array'] = []
    for f in jpegs:
        data['filename'].append(f)
        i = int(f.split('_')[-1].split('.')[0])
        data['id'].append(i)
        q = int(f.split('_')[2])
        data['quality'].append(q)
        data['size'].append(os.path.getsize(f))
        data['raw'].append(0)
        time_fname = args.save_dir + '/time_to_send_' + str(q) + '_' + str(i) + '.txt'
        time_to_send = 0
        with open(time_fname, 'r') as tf:
             time_to_send = float(tf.read());
        data['time'].append(time_to_send)
        jpeg = Image.open(f)
        d = np.array(jpeg.getdata()).reshape(jpeg.size[0], jpeg.size[1]) / 0xff
        d = cctf_encoding(demosaicing_CFA_Bayer_Menon2007(d, 'BGGR'))
        d = exposure.rescale_intensity(d, (0,1))
        #d = exposure.adjust_gamma(d, 1.5)
        #d = (d * 255).astype(np.uint8)
        fname = f.replace('mono', 'jpeg_demosaiced').replace('.jpeg', '.npy')
        np.save(fname, d)
        data['image_array'].append(fname)
    for f in raws:
        data['filename'].append(f)
        i = int(f.split('_')[-1].split('.')[0])
        data['id'].append(i)
        data['quality'].append(100)
        data['raw'].append(1)
        data['size'].append(d.shape[0] * d.shape[1])
        time_fname = args.save_dir + '/time_to_send_0_' + str(i) + '.txt'
        time_to_send = 0
        with open(time_fname, 'r') as tf:
             time_to_send = float(tf.read());
        data['time'].append(time_to_send)

        d = np.load(f) / float(0xff)
        d = cctf_encoding(demosaicing_CFA_Bayer_Menon2007(d, 'BGGR'))
        d = exposure.rescale_intensity(d, (0,1))
        #d = exposure.adjust_gamma(d, 1.5)
        #d = (d * 255).astype(np.uint8)
        fname = f.replace('raw', 'demosaiced')
        np.save(fname, d)
        data['image_array'].append(fname)

    data = pd.DataFrame(data)
    data.to_pickle(manifest_fname)
    print(data)
    print(np.unique(data['id']).shape)
else:
    print('Loading from file')
    data = pd.read_pickle(manifest_fname)
    print(data)
    print(np.unique(data['id']).shape)

if not os.path.isfile(detect_fname) :
    detector = ObjectDetection()
    detector.setModelTypeAsYOLOv3()
    detector.setModelPath("yolo.h5")
    detector.loadModel()

    detects = []

    for index, row in data.iterrows():
        image_array = (np.load(row['image_array']) * 255).astype(np.uint8)
        print(image_array)
        print(row['filename'])
        result_fname = args.save_dir + '/detection_' + str(row['quality']) + '_' + str(row['id']) + '.jpeg'
        detections = detector.detectObjectsFromImage(input_image=image_array, input_type="array", output_image_path=result_fname, minimum_percentage_probability=40)
        for eachObject in detections:
            print(eachObject["name"] , " : ", eachObject["percentage_probability"], " : ", eachObject["box_points"] )
            print("--------------------------------")
            eachObject['image_id'] = row['id']
            eachObject['quality'] = row['quality']
            detects.append(eachObject)

    dt = pd.DataFrame(detects)
    dt.to_pickle(detect_fname)

quality_group = data.groupby('quality')
print("mean:")
print(quality_group['size', 'time'].mean())
print("median:")
print(quality_group['size', 'time'].median())
print("std:")
print(quality_group['size', 'time'].std())
print("min:")
print(quality_group['size', 'time'].min())
print("max:")
print(quality_group['size', 'time'].max())


fig, ax = plt.subplots(1,figsize=(8,4))
plt.xlabel("Image Quality")

size_color = 'tab:blue'
time_color = 'tab:red'
energy_color = 'tab:green'
ax.set_ylabel('Image Size (kB)')
gm = quality_group.mean()
e = quality_group.std()['size']
ax.errorbar(gm.index, gm['size']/1E3, yerr=e/1E3, color=size_color)
fig.canvas.draw()
labels = ax.get_xticks().tolist()
labels = [str(int(x)) for x in labels]
labels[labels.index('100')] = 'raw'
ax.set_xticklabels(labels)
ax.grid(True)
plt.tight_layout()
plt.savefig('size_v_quality.png', dpi=180)

fig, ax = plt.subplots(1,figsize=(8,4))
plt.xlabel("Image Quality")
ax.set_ylabel('Time to Send (s)')
e = quality_group.std()['time']
ax.errorbar(gm.index, gm['time'], yerr=e, color=time_color)
fig.canvas.draw()
labels = ax.get_xticks().tolist()
labels = [str(int(x)) for x in labels]
labels[labels.index('100')] = 'raw'
ax.set_xticklabels(labels)
ax.grid(True)
plt.tight_layout()
plt.savefig('time_v_quality.png', dpi=180)

fig, ax = plt.subplots(1,figsize=(8,4))
plt.xlabel("Image Quality")
ax.set_ylabel('Estimated Energy (J)')
ax.plot(gm.index, gm['time']*4.8E-3*3.3, color=energy_color)
fig.canvas.draw()
labels = ax.get_xticks().tolist()
labels = [str(int(x)) for x in labels]
labels[labels.index('100')] = 'raw'
ax.set_xticklabels(labels)
ax.grid(True)
plt.tight_layout()
plt.savefig('energy_v_quality.png', dpi=180)
