EnHANTs Dataset Processing
==========================

[Download](http://enhants.ee.columbia.edu/indoor-irradiance-meas) and place
measurement `.txt.` files in the `enhants_data` directory.

The `clean.py` script takes these energy traces, finds periods of missing data,
generates synthetic replacements to fill these holes, and saves them as
`.npy` objects in the `numpy_arrays` directory to be consumed by the simulator.

