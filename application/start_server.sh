#!/bin/sh
RPI_IP=192.168.2.201
GDB_SERVER_PORT=1995

#sudo gdbserver $RPI_IP:$GDB_SERVER_PORT /home/pi/licenta/application/build/application 
sudo gdbserver raspberrypi.local:$GDB_SERVER_PORT /home/pi/licenta/application/build/application

