#!/bin/bash

set -e  # exit immediately on error
set -x  # display all commands

if [ ! -f libevent_install/lib/libevent.a ]; then
	if [ ! -f libevent-2.1.8-stable.tar.gz ]; then
		wget https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz
	fi	

	tar -xf libevent-2.1.8-stable.tar.gz
	cd libevent-2.1.8-stable

	./configure --prefix=`pwd`/../libevent_install
	make 
	make install
fi

echo "all done."
