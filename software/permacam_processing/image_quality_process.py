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

jpegs = glob(args.save_dir + '/*.jpeg')
raws = glob(args.save_dir + '/image_raw*.npy')
manifest = args.save_dir + '/manifest.pkl'
data = None
if not os.path.isfile(manifest) :
    print('Generating')
    data = {}
    data['filename'] = []
    data['id'] = []
    data['quality'] = []
    data['size'] = []
    data['raw'] = []
    data['image_array'] = []
    for f in jpegs:
        data['filename'].append(f)
        data['id'].append(int(f.split('_')[-1].split('.')[0]))
        data['quality'].append(int(f.split('_')[2]))
        data['size'].append(os.path.getsize(f))
        data['raw'].append(0)
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
        data['id'].append(int(f.split('_')[-1].split('.')[0]))
        data['quality'].append(0)
        data['raw'].append(1)
        d = np.load(f)
        data['size'].append(d.shape[0] * d.shape[1])

        d = cctf_encoding(demosaicing_CFA_Bayer_Menon2007(d, 'BGGR'))
        d = exposure.rescale_intensity(d, (0,1))
        #d = exposure.adjust_gamma(d, 1.5)
        #d = (d * 255).astype(np.uint8)
        fname = f.replace('raw', 'demosaiced')
        np.save(fname, d)
        data['image_array'].append(fname)

    data = pd.DataFrame(data)
    data.to_pickle(args.save_dir + '/manifest.pkl')
    print(data)
else:
    print('Loading from file')
    data = pd.read_pickle(manifest)
    print(data)

