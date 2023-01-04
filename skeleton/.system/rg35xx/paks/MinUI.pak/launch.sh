#!/bin/sh

export SDCARD_PATH="/mnt/sdcard"
export BIOS_PATH="$SDCARD_PATH/Bios"
export SAVES_PATH="$SDCARD_PATH/Saves"
export SYSTEM_PATH="$SDCARD_PATH/.system/rg35xx"
export CORES_PATH="$SYSTEM_PATH/cores"
export USERDATA_PATH="$SDCARD_PATH/.userdata/rg35xx"
export LOGS_PATH="$USERDATA_PATH/logs"

export PATH=$SYSTEM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:$LD_LIBRARY_PATH

mkdir -p "$LOGS_PATH"
mkdir -p "$USERDATA_PATH/.mmenu"
mkdir -p "$USERDATA_PATH/.minui"

echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

cd $(dirname "$0")

keymon.elf &

export EXEC_PATH=/tmp/minui_exec
touch "$EXEC_PATH" && sync

while [ -f "$EXEC_PATH" ]; do
	./minui.elf &> $LOGS_PATH/minui.txt
	sync
	
	NEXT="/tmp/next"
	if [ -f $NEXT ]; then
		CMD=`cat $NEXT`
		eval $CMD
		rm -f $NEXT
		# if [ -f "/tmp/using-swap" ]; then
		# 	swapoff $USERDATA_PATH/swapfile
		# 	rm -f "/tmp/using-swap"
		# fi
		
		# echo `date +'%F %T'` > "$DATETIME_PATH"
		sync
	fi
done

# this won't happen automatically because we're chroot-ed
rm -rf /tmp/*