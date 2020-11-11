#!/bin/bash
sudo make && sudo make install
sudo /etc/init.d/avahi-daemon restart -s --debug
sleep 5
while :
do
	if [ -s /browse_service.csv ]
	then
		
		sudo avahi-browse _chromium._tcp &
		sleep 10s
		sudo kill -9 $(ps -ef | grep "avahi-browse" | awk 'NR==1{print $2}')
		sleep 5s
	else
		sleep 20
	fi
done
