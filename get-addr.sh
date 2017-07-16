#!/bin/sh

# If the output file already exists, simply exit
[ -f "$2" ] && exit 0

prefix=`echo $1 | awk -F: '{print $1":"$2":"$3":"$4}'`

addr=`ip -6 addr | grep "$prefix" | grep 'global temporary' | awk '{print $2}'`

if [ -n "$addr" ]
then
	echo $addr | sed -e 's?/.*??' >"$2"
fi
