Lighting Test
=============

To read the light data from Permamote, use your favorite serial port monitor. Personally, I use miniterm, a python based serial reader.

Ensure python is installed. On Mac it should be preinstalled. Check with:
```
which python
```
If you get any output, you already have python installed.

To install miniterm, first make sure pip is installed:
```
which pip
```
If you get any output, you already have pip installed. If not, install it:
```
sudo easy_install pip
```
and install miniterm:
```
pip install pyserial
```

And now run this command with a Permamote connected through USB:
```
miniterm.py /dev/tty.usbserial* 115200
```
and you should get lux and color data every 5 seconds.

To save this data to a file instead of just terminal output, pipe it to a file:
```
miniterm.py /dev/tty.usbserial* 115200 > file.txt
```
