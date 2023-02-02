#!/bin/sh

touch /mnt/sdcard/batmon.txt
while :; do
	C=`cat /sys/class/power_supply/battery/capacity`
	V=`cat /sys/class/power_supply/battery/voltage_now`
	M=$(($V/1000))
	M=$(($M-3300))
	M=$(($M/8))
	
	if [ $M -gt 80 ]; 	then M=100;
	if [ $M -gt 60 ]; 	then M=80;
	elif [ $M -gt 40 ]; then M=60;
	elif [ $M -gt 20 ]; then M=40;
	elif [ $M -gt 10 ]; then M=20;
	else 					 M=10; fi
	
	N=`date`
	echo "$C ($M) $V $N" >> /mnt/sdcard/batmon.txt
	sync
	sleep 5
done