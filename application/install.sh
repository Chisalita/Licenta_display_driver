#!/bin/bash

FILES_DIR=$1

cp $FILES_DIR/startIliDriver /usr/bin/
chmod +x /usr/bin/startIliDriver
cp $FILES_DIR/iliDriver.service /etc/systemd/system

systemctl enable iliDriver.service
systemctl start iliDriver.service

#change the resolution
$FILES_DIR/change_resolution.sh