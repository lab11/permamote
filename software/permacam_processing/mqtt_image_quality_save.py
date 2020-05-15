#!/usr/bin/env python3

import json
import base64
from skimage import exposure
from io import BytesIO
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import scipy.misc
from PIL import Image
import numpy as np
from pathlib import Path
import argparse

parser = argparse.ArgumentParser(description='Demosaic Permacam images and republish color images')
parser.add_argument('-s', '--subscribe_host', default='localhost', help='Optional mqtt hostname to subscribe/publish from/to, default is localhost')
parser.add_argument('-t', '--topic', default='device/Permacam/#', help='Optional mqtt topic to subscribe/publish from/to, default is "device/Permacam/#"')
parser.add_argument('-d', '--save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

Path(args.save_dir).mkdir(parents=True, exist_ok=True)

def on_connect(client, userdata, flags, rc):
    print("connected with code " + str(rc))
    client.subscribe(args.topic)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode('utf-8'))
        print('got message ' + data['_meta']['topic'] + ' on ' + msg.topic)
        image_id = 0
        if 'image_id' in data:
            image_id = data['image_id']
        if data['_meta']['topic'] == 'image_jpeg':
            if 'image_is_demosaiced' in data and data['image_is_demosaiced']:
                # skip color images
                return
            # save mosaiced image
            fname = args.save_dir +  '/image_mono_' + str(data['image_jpeg_quality']) +'_'+ str(image_id) + '.jpeg'
            with open(fname, "wb") as f:
                f.write(base64.b64decode(data['image_jpeg']))
        elif data['_meta']['topic'] == 'image_raw':
                array = np.frombuffer(base64.b64decode(data['image_raw']), dtype=np.uint8).reshape(320,320)
                fname = args.save_dir +  '/image_raw_'+ str(image_id) + '.npy'
                np.save(fname, array)
        elif data['_meta']['topic'] == 'time_to_send_image':
            quality = 0
            if 'image_jpeg_quality' in data:
                quality = data['image_jpeg_quality']
            fname = args.save_dir +  '/time_to_send_' + str(quality) +'_'+ str(image_id) + '.txt'
            seconds = 0
            if 'time_to_send_s' in data:
                seconds = float(data['time_to_send_s']['low'])
            if 'time_to_send_us' in data:
                seconds += data['time_to_send_us'] / 1E6
            with open(fname, "w") as f:
                f.write(str(seconds))
    except Exception as e:
        print(e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(args.subscribe_host)

client.loop_forever()
