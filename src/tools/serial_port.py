#!/usr/bin/python3
import os
import serial
import time

# USB ACM device 波特率随便填
ser = serial.Serial("/dev/ttyACM0", 115200, timeout=0.5)

while(1):
    while(1):
        cube_string_kociemba = ser.read(54)
        print(cube_string_kociemba)
        if len(cube_string_kociemba) == 54:
            print('ok')
            break
        
    pipe = os.popen("kociemba " + cube_string_kociemba.decode(), 'r')
    solve = pipe.read()
    print(solve)
        
    while(1):
        skip = ser.read(1)
        print(skip)
        if skip == b':':
            break


        
    ser.write(bytes(solve,encoding = 'ascii'))
    ser.write(b'\r')

    while(1):
        skip = ser.read(1)
        print(skip)
        if skip == b'\n':
            break


ser.close()