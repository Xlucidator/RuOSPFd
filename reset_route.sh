#!/bin/bash 

# tmp
sudo route del -net 10.128.5.0 netmask 255.255.255.0 gw 192.168.75.4
sudo route del -net 192.168.75.0 netmask 255.255.255.0 gw 192.168.75.4