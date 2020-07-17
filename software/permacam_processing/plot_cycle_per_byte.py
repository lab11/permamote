#!/usr/bin/env python3

import matplotlib
font = {'family' : 'Arial',
        'weight' : 'medium',
        'size'   : 8}
matplotlib.rc('font', **font)
#matplotlib.use('Agg')
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

image_data = pd.read_pickle("power2/power_summary.pkl")
print(image_data)

mean  = image_data.mean(numeric_only=True)
print(mean)

# plot energy vs cycle per bit (byte?)
x = [i*10**exp for exp in range(0, 8) for i in range(1, 10)]

#send_160= [10.6E-3 for i in x]
send_320 = [mean['size'] / 320**2 * mean['e_per_bit']*8 for i in x]
energy_per_cycle = 3 * 52E-6 / 1E6
print(energy_per_cycle)
compute_line = [energy_per_cycle * i for i in x]

fig, ax = plt.subplots(1, figsize=(4,3))
ax.grid(True, which='major')

compress = (mean['t_compress'] / (1/64E6) / (320**2), mean['e_compress'] / 320**2)
ax.scatter(compress[0], compress[1], color='tab:red', zorder=3)
ax.annotate("JPEG Compression, Quality 90", (compress[0]*1.4, compress[1]*.75), weight='bold')

classification= (5.10 / (1/64E6) / (160**2), 50.9E-3 / 160**2)
ax.scatter(classification[0], classification[1], color='tab:orange', zorder=3)
ax.annotate("Mobilenet\nPerson Classification", (classification[0]*.6, classification[1]), weight='bold', ha='right')

detection = (5.41E9 * 7 / 320**2, 5.41E9 * 7 * energy_per_cycle / 320**2)
ax.scatter(detection[0], detection[1], color='tab:green', zorder=3)
ax.annotate("YOLOv3-tiny Detection", (detection[0]*0.7, detection[1]*.9), weight='bold', ha='right')

#ax.plot(x, send_160, color='black', ls='--', label='Energy to send 160', zorder=1)
ax.plot(x, send_320, color='black', ls='--', label='Transmission Energy per Byte', zorder=1)
#ax.plot(x, send_160_raw)
#ax.plot(x, send_320_raw)
ax.plot(x, compute_line, label='Processor Energy per Byte', zorder=2)

ax.loglog()
ax.set_xlim(x[0],x[-1])
ax.set_ylim(1E-9,1E-3)

ax.set_ylabel('Energy Per Byte (J/B)')
ax.set_xlabel('Processor Cycles Per Byte')
plt.legend(prop={'size': 6})
plt.tight_layout()
plt.savefig("cycles_per_byte.pdf")
#plt.show()
