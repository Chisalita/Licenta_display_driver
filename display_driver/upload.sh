#!/bin/sh

RPI_IP=192.168.2.201

scp *.ko pi@$RPI_IP:~/lkm

