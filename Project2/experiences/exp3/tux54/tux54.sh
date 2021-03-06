#!/bin/sh

ifconfig eth1 up
ifconfig eth1 172.16.51.253/24

echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

route del default gw 172.16.50.1

# configure terminal
# interface fastethernet 0/7
# switchport mode access
# switchport access vlan 51
# end
# show vlan brief

#clean arp tables after pinging
