#!/bin/bash

# non-persisten tun setup
# tun device must be open first

IFCONFIG="/sbin/ifconfig tun0"
ISIT=`$IFCONFIG | grep "UP" | wc -l`;

if test $ISIT -gt 0;
 then
    echo "tun0 is already up!";
else
    echo "Setting up tun0 and running the test...";
#    ifconfig tun0 0.0.0.0 up    -- server does this
#    sleep 1s
    route add -net 10.0.2.0 gw 192.168.2.22 netmask 255.255.255.0 eth0 
#    route add 10.0.2.0 dev tun0
fi

#./network_client 10.10.10.1 3001 10 0
./network_client 10.0.2.8 3001 10 0
