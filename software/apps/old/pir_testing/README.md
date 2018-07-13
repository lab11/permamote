Occupancy Test
=============

To read the occupancy data from Permamote, The script `read_pir_output.py` is
your friend.

Ensure python is installed. On Mac it should be preinstalled if you're on
MacOS. Check with:
```
which python
```
If you get any output, you already have python installed.

To install pyserial, first make sure pip is installed:
```
which pip
```
If you get any output, you already have pip installed. If not, install it:
```
sudo easy_install pip
```
and install pyserial:
```
pip install pyserial
```

And now run this command with a Permamote connected through USB:
```
./read_pir_output.py -i /dev/tty.usbserial*
```

and you should get motion events on every detection.

To save this data to a file instead of just terminal output, pipe it to a file:
```
./read_pir_output.py -i /dev/tty.usbserial* > file.txt
```
