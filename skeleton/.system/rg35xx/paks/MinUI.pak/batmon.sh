#!/bin/sh

touch /mnt/sdcard/batmon.txt
while :; do
	echo `cat /sys/class/power_supply/battery/capacity` `cat /sys/class/power_supply/battery/voltage_now` `date` >> /mnt/sdcard/batmon.txt
	sync
	sleep 5
done