#!/usr/bin/env python3

import json
import base64
from skimage import exposure
from io import BytesIO
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import colour
import scipy.misc
from PIL import Image
import numpy as np
from pathlib import Path
import argparse

parser = argparse.ArgumentParser(description='Demosaic Permacam images and republish color images')
parser.add_argument('-s', '--subscribe_host', default='localhost', help='Optional mqtt hostname to subscribe/publish from/to, default is localhost')
parser.add_argument('-p', '--publish_host', default='localhost', help='Optional mqtt hostname to subscribe/publish from/to, default is localhost')
parser.add_argument('-t', '--topic', default='device/Permacam/#', help='Optional mqtt topic to subscribe/publish from/to, default is "device/Permacam/#"')
args = parser.parse_args()

from colour_demosaicing import (
    EXAMPLES_RESOURCES_DIRECTORY,
    demosaicing_CFA_Bayer_bilinear,
    demosaicing_CFA_Bayer_Malvar2004,
    demosaicing_CFA_Bayer_Menon2007,
    mosaicing_CFA_Bayer)

cctf_encoding = colour.cctf_encoding

def on_connect(client, userdata, flags, rc):
    print("connected with code " + str(rc))
    client.subscribe(args.topic)

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode('utf-8'))
        print('got message ' + data['_meta']['topic'] + ' on ' + msg.topic)
        if data['_meta']['topic'] == 'image_jpeg':
            if 'image_is_demosaiced' in data and data['image_is_demosaiced']:
                # skip color images
                return

            jpeg = Image.open(BytesIO(base64.b64decode(data['image_jpeg'])))
            image_arr = np.array(jpeg.getdata()).reshape(jpeg.size[0], jpeg.size[1]) / 0xff
            image_arr = cctf_encoding(demosaicing_CFA_Bayer_Menon2007(image_arr, 'BGGR'))
            image_arr = exposure.rescale_intensity(image_arr, (0,1))
            image_arr = exposure.adjust_gamma(image_arr, 1.5)
            im = Image.fromarray((image_arr*255).astype(np.uint8))
            buffered = BytesIO()
            im.save(buffered, format="JPEG")
            im_str = base64.b64encode(buffered.getvalue()).decode('ascii')
            data['image_jpeg'] = im_str
            data['image_is_demosaiced'] = True
            new_topic = args.topic.replace('#', data['_meta']['device_id'])
            publish.single(new_topic, json.dumps(data), hostname=args.publish_host)

    except Exception as e:
        print(e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(args.subscribe_host)

client.loop_forever()
