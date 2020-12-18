#!/bin/bash

while :
do
	sleep 5
	avahi-publish -s rasspb170_1-airplay-srv-host _airplay._tcp 1 rasspb170_1-airplay-txt-host &
	sleep 10
	kill -9 $(ps -ef | grep "avahi-publish" | awk 'NR==1{print $2}')
	sleep 1
	service avahi-daemon restart
done
