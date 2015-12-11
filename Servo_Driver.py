#!/usr/bin/env python

import RPi.GPIO as GPIO
import time
import socket
import os

host = ''
port = 1234
backlog = 5
size = 1024

print('Listening on port', port)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host,port))
s.listen(backlog)


os.system("echo 0=150 > /dev/servoblaster") # X-Axis
os.system("echo 1=230 > /dev/servoblaster") # Y-Axis
try:
        client, address = s.accept()
        print('Binded to client')
        while True:

                data = client.recv(size)
                if data:
                        try:
                                #print data.split()

                                input = float(data)

                                #Left
                                if input == 1:
                                        print('left')
                                        os.system("echo 0=+10 > /dev/servoblaster")
                                #Right
                                if input == 2:
                                        print('right')
                                        os.system("echo 0=-10 > /dev/servoblaster")
                                #Up 
                                if input == 4:
                                        print('up')
                                        os.system("echo 1=+5 > /dev/servoblaster")
                                #Down
                                if input == 3:
                                        os.system("echo 1=-5 > /dev/servoblaster")
                                        print('down')

                                #Upper-Left
                                if input == 10:
                                        os.system("echo 1=-5 > /dev/servoblaster")
                                        os.system("echo 0=+10 > /dev/servoblaster")
                                        print('UP-LEFT')

                                #Upper-Right
                                if input == 11:
                                        os.system("echo 1=-5 > /dev/servoblaster")
                                        os.system("echo 0=-10 > /dev/servoblaster")
                                        print('UP-Right')
                                #Down-Left
                                if input == 12:
                                        os.system("echo 1=+5 > /dev/servoblaster")
                                        os.system("echo 0=+10 > /dev/servoblaster")
                                        print('DWN-LEFT')

                                #Down-Right
                                if input == 13:
                                        os.system("echo 1=+5 > /dev/servoblaster")
                                        os.system("echo 0=-10 > /dev/servoblaster")

                                #print('X: ', x_value)
                                #print('Y: ', y_value)

except KeyboardInterrupt:
        GPIO.cleanup()






