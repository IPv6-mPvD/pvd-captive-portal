#!/bin/sh

#set -x
IPv6_ADDR=$1
PROFILE=$2


# output is like 2001:67c:1230:abba:f0d2:7ede:6684:e850 dev wlan0 lladdr 00:e0:8f:00:9c:41 used 0/0/0 probes 3 FAILED
MAC_ADDR=`/sbin/ip -6 neigh show $IPv6_ADDR | grep $IPv6_ADDR | /usr/bin/awk '{print $5}'`
INTFC=`/sbin/ip -6 neigh show $IPv6_ADDR | grep $IPv6_ADDR | /usr/bin/awk '{print $3}'`

if [ -z "$MAC_ADDR" ] ; then
	echo Cannot find the MAC address of $IPv6_ADDR
	exit ; fi

echo Found the MAC address of $IPv6_ADDR as $MAC_ADDR in interface $INTFC

#/usr/sbin/chilli_query authorize mac $MAC_ADDR

set -x
/usr/sbin/iptables -D FORWARD -i $INTFC --match mac --mac-source $MAC_ADDR -j ACCEPT
/usr/sbin/iptables -I FORWARD -i $INTFC --match mac --mac-source $MAC_ADDR -j ACCEPT
/usr/sbin/ip6tables -D FORWARD -i $INTFC --match mac --mac-source $MAC_ADDR -j ACCEPT
/usr/sbin/ip6tables -I FORWARD -i $INTFC --match mac --mac-source $MAC_ADDR -j ACCEPT
