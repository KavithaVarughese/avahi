#!/bin/bash

sudo make && sudo make install
sudo /etc/init.d/avahi-daemon restart -s --debug
sleep 5

while :
do
	if [ -s /ip.csv ]
	then
		
		sudo avahi-publish -s rasspb170_1-airplay-srv-host _airplay._tcp 1 rasspb170_1-airplay-txt-host &
		sleep 15
		sudo kill -9 $(ps -ef | grep "avahi-publish" | awk 'NR==1{print $2}')
	else
		sleep 20
	fi
done
