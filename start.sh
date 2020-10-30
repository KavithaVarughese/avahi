#!/bin/bash
sudo make && sudo make install
while :
do
	if [ -s /browse_service.csv ]
	then
		sudo /etc/init.d/avahi-daemon restart -s --debug
		sleep 5
		sudo avahi-browse _chromium._tcp &
		sleep 15
		sudo kill -9 $(ps -ef | grep "avahi-browse" | awk 'NR==1{print $2}')
	else
		sleep 20
	fi
done
