Permamote Openthread COAP App
=============================

This is the default app for Permamote. It utilizes
[OpenThread](https://github.com/openthread/openthread) to establish data
backhaul over COAP and services such as NTP and DNS.
See the
[tutorial](https://github.com/lab11/permamote/tree/master/tutorial) to set up
your own OpenThread network.
This app periodically sends sensor data from the
light color, temperature,
humidity and pressure sensors. The light and motion sensor send when
changes are detected. The acceleratometer is currently not implemented. Thread
configuration, Sensor periods/backoffs, and COAP, NTP, and DNS endpoints, are
defined in
[config.h](https://github.com/lab11/permamote/blob/master/software/apps/permamote_coap/config.h).

This application is configured to use Nordic's [background
bootloader](
   https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/lib_background_dfu.html
    ).
Before flashing this app, make sure to generate keys, copy the keys, and flash the bootloader once.

To generate keys, follow Nordic's process [here](
    https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.thread_zigbee.v3.0.0/thread_example_dfu.html
    ).
```
nrfutil keys generate private.pem
nrfutil keys display --key pk --format code private.pem --out_file public.c
cp private.pem public.c ../bg_bootloader/.
```

Navigate to the
[`bg_bootloader`](https://github.com/lab11/permamote/tree/master/software/apps/bg_bootloader) directory and:
```
make flash
```

You should see a red LED turn on. This means the bootloader has successfully been flashed and recognizes that there is not yet a valid application installed.

Next, navigate to the [`permamote_coap`](https://github.com/lab11/permamote/tree/master/software/apps/permamote_coap) application and flash this app, with a device ID:
```
make flash ID=C0:98:E5:11:XX:XX
```
Replace XX:XX with your desired ID.


