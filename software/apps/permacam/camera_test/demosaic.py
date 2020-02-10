#!/usr/bin/env python3

import colour
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

image = np.fromfile('image.raw', 'uint8')/0xff
print(image[0])
print(np.average(image))
print(image.shape)
print(image.shape[0]/324)
lines = int(image.shape[0]/324)
image = np.reshape(image, (lines, 324))
print(image.shape)
encoded = cctf_encoding(image)
plot_image(encoded)
plot_image(cctf_encoding(demosaicing_CFA_Bayer_Menon2007(image, 'BGGR')))
