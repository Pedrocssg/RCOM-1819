#!/bin/sh

/etc/init.d/networking restart
ifconfig eth0 up
ifconfig eth0 172.16.51.1/24
#connect cables
