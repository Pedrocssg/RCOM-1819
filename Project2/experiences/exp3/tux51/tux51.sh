#!/bin/sh

route add -net 172.16.51.0/24 gw 172.16.50.254

ping 172.16.50.254
ping 172.16.51.253
ping 172.16.51.1

#clean arp tables after pinging
