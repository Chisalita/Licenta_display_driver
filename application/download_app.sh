#!/bin/sh

RPI_IP=192.168.2.201

scp -r pi@raspberrypi.local:~/licenta/application/build/application ./build/application
