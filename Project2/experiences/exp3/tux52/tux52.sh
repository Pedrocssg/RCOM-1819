#!/bin/sh

route add default gw 172.16.51.253
route add -net 172.16.50.0/24 gw 172.16.51.253

#clean arp tables after pinging
