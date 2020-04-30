#!/bin/bash

# qtwsserver -p 8888 -k /home/botho/git/LibCMVC-WS/qtwsserver/localhost.key  -c /home/botho/git/LibCMVC-WS/qtwsserver/localhost.cert -s 8889 &

for A in `seq 0 1`
do
	for B in `seq 0 5`
	do
		for C in `seq 0 9`
		do
			for D in `seq 0 9`
			do
				CMD="qtwsserver -p 1$A$B$C$D -k /home/botho/git/LibCMVC-WS/qtwsserver/localhost.key -c /home/botho/git/LibCMVC-WS/qtwsserver/localhost.cert -s 2$A$B$C$D -b ws://localhost:8888"
				echo "$CMD"
				$CMD &
			done
		done
	done
done


