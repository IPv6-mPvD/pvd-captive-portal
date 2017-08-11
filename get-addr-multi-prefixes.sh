#!/bin/sh

# usage : get-addr.sh interface [prefix]*
# This script is not meant to be used for a general usage
# It is intended to be detect any potential assigned IPv6 address
# corresponding to one of the prefix
# If an address is found, it is printed on the standard output
#
# In the context of the captive portal POC, prefixes are supposed
# to be /64
#
# Gets called by pvdSADR.js

interface="$1"
shift

for p
do
	# Just keep the /64 relevant part of the prefix
	prefix=`echo $p | awk -F: '{print $1":"$2":"$3":"$4}'`

	addr=`ip -6 addr show dev "$interface" |
	      grep "$prefix" |
	      grep 'global temporary' |
	      awk '{print $2}'`

	if [ -n "$addr" ]
	then
		# Remove the /<length> trailing part of the address
		echo $addr | sed -e 's?/.*??'
		exit 0
	fi
done
