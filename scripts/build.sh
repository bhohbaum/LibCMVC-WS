#!/bin/bash

PWD=`pwd`

cd /home/botho/git/build-qtws-Desktop_Qt_5_13_0_GCC_64bit-Release
make clean
make qmake_all
make -j 4
sudo make install
cd ..
scp -r LibCMVC-WS root@vweb05.kmundp.de:/root/tmp/
ssh root@vweb05.kmundp.de "bash -c 'cd /root/tmp/build ; make clean ; make qmake_all ; make ; make install ; killall qtwsserver ; /root/scripts/benimble/wsloop.sh &'"

cd $PWD

