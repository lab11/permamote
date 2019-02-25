Permamote Thread Test
=====================

Test OpenThread on Permamote. Basically the `do_nothing_well` application, but
with OpenThread implemented as well.

Expected RTT output (`make rtt`) with a correctly configured OpenThread network should be similar to:
```
<info> app: Thread version: OPENTHREAD/20170716-01135-gedb7982f; NRF52840; Dec 11 2018 22:05:35
<info> app: Network name:   OpenThread
<info> app: Thread Channel: 25
<info> app: TX Power: 0 dBm
<info> app: Thread PANID: 0xFACE
<info> app: Thread interface has been enabled.
<info> app: 802.15.4 Channel: 25
<info> app: 802.15.4 PAN ID:  0xFACE
<info> app: rx-on-when-idle:  disabled
<info> app: State changed! Flags: 0x045F133D Current role: 1

<info> app: State changed! Flags: 0x000000E4 Current role: 2

<info> app: State changed! Flags: 0x00000200 Current role: 2

<info> app: State changed! Flags: 0x00000001 Current role: 2
```
