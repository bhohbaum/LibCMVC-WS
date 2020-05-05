#!/bin/bash

DELAY=8

while true
do
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://vweb05.kmundp.de:8888/global -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://vweb05.kmundp.de:8888/asdf -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://vweb05.kmundp.de:8888/chat1 -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://vweb05.kmundp.de:8888/chat2 -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://localhost:3888/chat2 -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://localhost:3888/asdf -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://localhost:3888/global -c
	sleep $DELAY
	echo -n "das ist ein test"`sleep 0` | qtwsclient ws://localhost:5888/chat1 -c
	sleep $DELAY
done


