#!/bin/bash

PWD=`pwd`

sudo echo ok

cd /home/botho/git/build-qtws-Desktop_Qt_5_13_0_GCC_64bit-Release
make clean
make qmake_all
make -j 4
sudo make install
sudo killall qtwsserver

cd ..
rsync -avoze ssh LibCMVC-WS root@vweb05.kmundp.de:/root/tmp/
rsync -avoze ssh --delete LibCMVC-WS root@vweb05.kmundp.de:/root/tmp/
ssh root@vweb05.kmundp.de "/bin/bash -c 'cd /root/tmp/build ; make clean ; make qmake_all ; make -j 4 ; make install ; killall qtwsserver'"
ssh root@vweb05.kmundp.de "/bin/bash -c 'rm -v /var/www/benimble/bin/qtws* ; cp -arv /usr/local/bin/qtws* /var/www/benimble/bin/ '"
ssh root@vweb05.kmundp.de "/root/scripts/benimble/wsloop.sh & exit" &
sleep 5
kill -9 `ps -ef | grep "[s]sh root@vweb05.kmundp.de" | awk '{print $2}'`

scp -r root@vweb05.kmundp.de:/var/www/benimble/bin/qtws* /home/botho/git/joe-nimble-app-backend-cms/bin/

if [ ! -z "$1" ]
then
	cd /home/botho/git/joe-nimble-app-backend-cms/
	git add ./bin/qtws*
	git commit -m "$@"
	git pull
	git push

	cd /home/botho/git/LibCMVC-WS/
	git add .
	git commit -m "$@"
	git pull
	git push
fi

cd $PWD


