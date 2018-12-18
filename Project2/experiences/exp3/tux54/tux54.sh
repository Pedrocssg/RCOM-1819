#!/bin/sh

ifconfig eth1 up
ifconfig eth1 172.16.51.253

echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

# configure terminal
# interface fastethernet 0/4
# switchport mode access
# switchport access vlan 51
# end
# show vlan brief
