#!/bin/bash

PWD=`pwd`

cd /home/botho/git/build-qtws-Desktop_Qt_5_13_0_GCC_64bit-Release
make clean
make qmake_all
make -j 4
sudo make install
sudo killall qtwsserver

cd ..
scp -r LibCMVC-WS root@vweb05.kmundp.de:/root/tmp/
ssh root@vweb05.kmundp.de "/bin/bash -c 'cd /root/tmp/build ; make clean ; make qmake_all ; make -j 4 ; make install ; killall qtwsserver'"
ssh root@vweb05.kmundp.de "/bin/bash -c 'rm -v /var/www/benimble/bin/qtws* ; cp -arv /usr/local/bin/qtws* /var/www/benimble/bin/ '"
ssh root@vweb05.kmundp.de "/root/scripts/benimble/wsloop.sh &"

scp -r root@vweb05.kmundp.de:/var/www/benimble/bin/qtws* /home/botho/git/joe-nimble-app-backend-cms/bin/
cd /home/botho/git/joe-nimble-app-backend-cms/
git add ./bin/qtws*
git commit -m "websocket server/client binaries updated."
git pull
git push

cd $PWD


