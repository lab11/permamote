#! /usr/bin/env python3

import argparse
from config import default_config
import numpy as np
from simulate import simulate

# Input files for simulation
parser = argparse.ArgumentParser(description='Energy Harvesting Simulation.')
parser.add_argument('lightfile', metavar='l', type=str, help='A file to use to simulate light conditions')
parser.add_argument('pirfile', metavar='p', type=str, help='A file to use to simulate wake up events from a PIR sensor')
parser.add_argument('numdays', metavar='n', type=int, help='Number of days to simulate')
args = parser.parse_args()

# load light and motion files
lights = np.load(args.lightfile)
motions = np.load(args.pirfile)

# load default config
config = default_config
design_config = default_config.design_config
primary_config = config.primary_config
if 'secondary' in design_config:
    secondary_config = config.secondary_configs[design_config['secondary']]
else:
    secondary_config = 0

solar_config = config.solar_config

simulate(config, lights, motions, args.numdays);
