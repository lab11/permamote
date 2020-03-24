#!/usr/bin/env python3

import json
import base64
from skimage import exposure
from io import BytesIO
import paho.mqtt.client as mqtt
import colour
import scipy.misc
from PIL import Image
import numpy as np
from matplotlib import pyplot as plt
import matplotlib
from colour.plotting import *
from pathlib import Path

from colour_demosaicing import (
    EXAMPLES_RESOURCES_DIRECTORY,
    demosaicing_CFA_Bayer_bilinear,
    demosaicing_CFA_Bayer_Malvar2004,
    demosaicing_CFA_Bayer_Menon2007,
    mosaicing_CFA_Bayer)

cctf_encoding = colour.cctf_encoding

Path("jpeg").mkdir(parents=True, exist_ok=True)

def on_connect(client, userdata, flags, rc):
    print("connected with code " + str(rc))
    client.subscribe('device/Permacam/#')

def rgb_clip(image):
    r = image[:,:,0]
    image[:,:,0] = (r - r.min()) * (1/(r.max() - r.min()) * 255)
    r = image[:,:,1]
    image[:,:,1] = (r - r.min()) * (1/(r.max() - r.min()) * 255)
    r = image[:,:,2]
    image[:,:,2] = (r - r.min()) * (1/(r.max() - r.min()) * 255)
    print(image)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode('utf-8'))
        print('got message ' + data['_meta']['topic'])
        seq_no = 0
        if 'seq_no' in data:
            seq_no = data['seq_no']
        if data['_meta']['topic'] == 'image_jpeg':
            fname = 'jpeg/image_mono_' + str(data['image_jpeg_quality']) +'_'+ str(seq_no) + '.jpeg'
            with open(fname, "wb") as f:
                f.write(base64.b64decode(data['image_jpeg']))
            fname = 'jpeg/image_color_' + str(data['image_jpeg_quality']) +'_'+ str(seq_no) + '.jpeg'
            jpeg = Image.open(BytesIO(base64.b64decode(data['image_jpeg'])))
            jpeg_np = np.array(jpeg.getdata()).reshape(jpeg.size[0], jpeg.size[1]) / 0xff
            image_arr = cctf_encoding(demosaicing_CFA_Bayer_Menon2007(jpeg_np, 'BGGR'))
            image_arr = exposure.rescale_intensity(image_arr, (0,1))
            image_arr = exposure.adjust_gamma(image_arr, 1.5)
            im = Image.fromarray((image_arr*255).astype(np.uint8))
            im.save(fname)
    except Exception as e:
        print(e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("coap-test.permamote.com")

client.loop_forever()
