#!/bin/sh

ifconfig eth0 up
ifconfig eth0 172.16.50.1/24
route add default gw 172.16.50.254
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
