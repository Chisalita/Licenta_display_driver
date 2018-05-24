#!/bin/sh
RPI_IP=192.168.2.241

#  scp -r `pwd` root@$RPI_IP:~/licenta/app
scp -r `pwd` pi@raspberrypi.local:~/licenta

