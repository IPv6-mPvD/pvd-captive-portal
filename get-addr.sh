#!/bin/sh

# usage : get-addr.sh prefix outfile
# This script is not meant to be used for a general usage
# It is intended to be called in a loop with multi prefixes,
# one prefix at a time : it attempts to find an assigned IPv6
# address corresponding to the prefix and writes its to the
# output file. If the output file does exist, this means that
# a previous match has been found => dont try to find a match
# any more (the first one is enough)
#
# In the context of the captive portal POC, prefixes are supposed
# to be /64
#
# Gets called by pvdHttpServer.js

# If the output file already exists, simply exit
[ -f "$2" ] && exit 0

# Just keep the /64 relevant part of the prefix
prefix=`echo $1 | awk -F: '{print $1":"$2":"$3":"$4}'`

addr=`ip -6 addr | grep "$prefix" | grep 'global temporary' | awk '{print $2}'`

if [ -n "$addr" ]
then
	# Remove the /<length> trailing part of the address
	echo $addr | sed -e 's?/.*??' >"$2"
fi
