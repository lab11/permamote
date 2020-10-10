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
import time

parser = argparse.ArgumentParser(description='Perform object detection on demosaiced Permacam images')
parser.add_argument('-s', '--subscribe_host', default='localhost', help='Optional mqtt hostname to subscribe/publish from/to, default is localhost')
parser.add_argument('-p', '--publish_host', default='localhost', help='Optional mqtt hostname to subscribe/publish from/to, default is localhost')
parser.add_argument('-t', '--topic', default='device/Permacam/#', help='Optional mqtt topic to subscribe/publish from/to, default is "device/Permacam/#"')
parser.add_argument('-d', '--save_dir', help='Optional save detected images to file in dir')
args = parser.parse_args()

from imageai.Detection import ObjectDetection
detector = ObjectDetection()
detector.setModelTypeAsYOLOv3()
detector.setModelPath("/etc/imageai/yolo.h5")
detector.loadModel()

if args.save_dir:
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
            if 'image_is_demosaiced' not in data or not data['image_is_demosaiced']:
                return

            jpeg = BytesIO(base64.b64decode(data['image_jpeg']))

            start = time.time()
            result, detections = detector.detectObjectsFromImage(input_image=jpeg, input_type="stream", output_type="array", minimum_percentage_probability=30)
            print(time.time() - start)
            for eachObject in detections:
                print(eachObject["name"] , " : ", eachObject["percentage_probability"], " : ", eachObject["box_points"] )
                print("--------------------------------")
            print(detections)
            result = Image.fromarray(result)
            stream = BytesIO()
            result.save(stream, format='JPEG')

            if (args.save_dir):
                fname = args.save_dir + '/image_detect_' + str(data['image_jpeg_quality']) +'_'+ str(image_id) + '.jpeg'
                result.save(fname, format='JPEG')

            im_str = base64.b64encode(stream.getvalue()).decode('ascii')

            new_topic = args.topic.replace('#', data['_meta']['device_id'])
            new_data = {}
            new_data['_meta'] = data['_meta']
            new_data['image_id'] = data['image_id']
            new_data['image_jpeg_quality'] = data['image_jpeg_quality']
            new_data['seq_no'] = data['seq_no']
            
            new_data['_meta']['topic'] = 'image_detection_jpeg'
            new_data['image_jpeg'] = im_str 
            new_data['image_is_demosaiced'] = 1
            publish.single(new_topic, json.dumps(new_data), hostname=args.publish_host.strip())
            new_data.pop('image_jpeg')
            new_data.pop('image_is_demosaiced')
            new_data['_meta']['topic'] = 'image_detections'
            for detection in detections:
                new_data['image_detection'] = detection['name']
                new_data['image_detection_probability'] = float(detection['percentage_probability'])
                for i, point in enumerate(detection['box_points']):
                    axis = 'x'
                    if i % 2:
                        axis = 'y'
                    new_data['image_detection_box_point_' + axis + str(int(i/2.0) + 1)] = int(point)
                print(new_data)
                publish.single(new_topic, json.dumps(new_data), hostname=args.publish_host)


    except Exception as e:
        print(e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(args.subscribe_host.strip())

client.loop_forever()
