#!/bin/sh

# usage : get-local-addr.sh devname outfile
# This script is not meant to be used for a general usage
# Its purpose is to find the 1st suitable local address on
# a given interface
#
# Gets called by pvdHttpServer.js

# If the output file already exists, simply exit
[ -f "$2" ] && exit 0

addr=`ip -6 addr show "$1" | grep 'scope link' | awk '{print $2}'`

if [ -n "$addr" ]
then
	# Remove the /<length> trailing part of the address
	echo $addr | sed -e 's?/.*??' >"$2"
fi
