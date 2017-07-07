#! /usr/bin/env python3

import numpy as np

active_current = 10E-3
sleep_current = 5E-6

solar_panel_eff = .19 #%
solar_panel_area = 14 #cm^2

def lux2irradiance(lux):
    # irradiance estimation from lux using values reported on page 202 in:
    # http://www.bradcampbell.com/wp-content/uploads/2012/05/yerva-ipsn12-energy_harvesting_leaves.pdf
    lux[lux < 75] = lux[lux < 75]*18.6/50
    lux[np.logical_and(lux >= 75, lux < 200)] = lux[np.logical_and(lux >= 75, lux < 200)]*29.1/100
    lux[lux > 200] = lux[lux > 200]*74.9/320

