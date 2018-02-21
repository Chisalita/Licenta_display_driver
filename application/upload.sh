#!/bin/sh
RPI_IP=192.168.2.201

scp -r `pwd` pi@$RPI_IP:~/licenta

