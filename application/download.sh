#!/bin/sh

RPI_IP=192.168.2.201

scp -r pi@$RPI_IP:~/licenta/application/* ./
