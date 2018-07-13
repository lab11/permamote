#! /usr/bin/env python3

try:
    import serial
except:
    print('You must install pyserial:\npip3 install pyserial')
import datetime
import argparse

parser = argparse.ArgumentParser(description='PIR data reader.')
parser.add_argument('-i', dest='interface', default='/dev/ttyUSB0', help='USB serial interface. i.e. /dev/ttyUSB0')
args = parser.parse_args()

ser = serial.Serial(args.interface, 115200)

while 1:
    line = ser.readline().decode('utf-8').split('\r\n')[0]
    if len(line) != 0:
        line += ' ' + str(datetime.datetime.now())
        print(line)
