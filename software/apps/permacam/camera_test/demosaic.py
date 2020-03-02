#!/usr/bin/env python3

import colour
from PIL import Image
import numpy as np
from matplotlib import pyplot as plt
from colour.plotting import *

from colour_demosaicing import (
    EXAMPLES_RESOURCES_DIRECTORY,
    demosaicing_CFA_Bayer_bilinear,
    demosaicing_CFA_Bayer_Malvar2004,
    demosaicing_CFA_Bayer_Menon2007,
    mosaicing_CFA_Bayer)

cctf_encoding = colour.cctf_encoding

image = np.fromfile('image.raw', 'uint8')[:320*320]/0xff
print(image[0])
print(np.average(image))
print(image.shape)
print(image.shape[0]/320)
lines = int(image.shape[0]/320)
image = np.reshape(image, (lines, 320))
print(image.shape)

im = Image.open("image.jpeg")
image_jpeg = np.array(im.getdata()).reshape(im.size[0], im.size[1])/0xff

encoded = cctf_encoding(image)
encoded_jpeg = cctf_encoding(image_jpeg)
plot_image(encoded)
plot_image(encoded_jpeg)
plot_image(cctf_encoding(demosaicing_CFA_Bayer_Menon2007(image, 'BGGR')))
plot_image(cctf_encoding(demosaicing_CFA_Bayer_Menon2007(image_jpeg, 'BGGR')))
