#!/usr/bin/env python3

import argparse
from glob import glob
from skimage import exposure
from PIL import Image
import colour
import numpy as np
import pandas as pd
import os

from colour_demosaicing import (
    demosaicing_CFA_Bayer_bilinear,
    demosaicing_CFA_Bayer_Malvar2004,
    demosaicing_CFA_Bayer_Menon2007,
    mosaicing_CFA_Bayer)
cctf_encoding = colour.cctf_encoding

parser = argparse.ArgumentParser(description='Process quality series of Permacam images and generate error measurements')
parser.add_argument('save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

jpegs = glob(args.save_dir + '/image_mono*.jpeg')
raws = glob(args.save_dir + '/image_raw*.npy')
manifest_fname = args.save_dir + '/manifest.pkl'
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
    d = None
    fname = None
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
        d = exposure.adjust_gamma(d, 1.5)
        #d = (d * 255).astype(np.uint8)
        fname = f.replace('mono', 'jpeg_demosaiced').replace('.jpeg', '.npy')

        np.save(fname, d)
        data['image_array'].append(fname)
        result = Image.fromarray((d * 255).astype(np.uint8))
        result.save(fname.replace('npy','jpeg'), format='JPEG')

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
        d = exposure.adjust_gamma(d, 1.5)
        #d = (d * 255).astype(np.uint8)
        fname = f.replace('raw', 'demosaiced')

        np.save(fname, d)
        data['image_array'].append(fname)
        result = Image.fromarray((d * 255).astype(np.uint8))
        result.save(fname.replace('npy','jpeg'), format='JPEG')

    data = pd.DataFrame(data)
    qualities = set(np.unique(data['quality']).flatten())
    for i in data['id']:
        avail_qual = set(data[data['id'] == i]['quality'])
        if (avail_qual != qualities):
            data = data[data['id'] != i]
    data.to_pickle(manifest_fname)
else:
    data = pd.read_pickle(manifest_fname)

print(data)
print(len(np.unique(data['id'])))
