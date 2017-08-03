#!/bin/sh

# usage : get-local-addr.sh devname outfile
# This script is not meant to be used for a general usage
# Its purpose is to find the 1st suitable local address on
# a given interface (ipv6 and ipv4)
#
# Gets called by pvdHttpServer.js

getaddr() {
	ip -$1 addr show "$3" 2>/dev/null |
	grep $2 |
	head -1 |
	awk '{print $2}' |
	sed -e 's?/.*??'
}

addr6=`getaddr 6 inet6 "$1"`
addr4=`getaddr 4 inet "$1"`

[ -z "${addr4}${addr6}" ] && exit 0

{
	[ -z "$addr6" ] || echo "$addr6"
	[ -z "$addr4" ] || echo "$addr4"
} >"$2"
