#!/bin/sh

export SDCARD_PATH="/mnt/sdcard"
export BIOS_PATH="$SDCARD_PATH/Bios"
export SAVES_PATH="$SDCARD_PATH/Saves"
export SYSTEM_PATH="$SDCARD_PATH/.system/rg35xx"
export CORES_PATH="$SYSTEM_PATH/cores"
export USERDATA_PATH="$SDCARD_PATH/.userdata/rg35xx"
export LOGS_PATH="$USERDATA_PATH/logs"

#######################################

export PATH=$SYSTEM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:$LD_LIBRARY_PATH

#######################################

echo on > /sys/devices/b0238000.mmc/mmc_host/mmc0/power/control
echo on > /sys/devices/b0230000.mmc/mmc_host/mmc1/power/control

export CPU_SPEED_MENU=504000
export CPU_SPEED_GAME=1296000
export CPU_SPEED_PERF=1488000
export CPU_PATH=/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

#######################################

mkdir -p "$LOGS_PATH"
mkdir -p "$USERDATA_PATH/.minui"
AUTO_PATH=$USERDATA_PATH/auto.sh
if [ -f "$AUTO_PATH" ]; then
	"$AUTO_PATH"
fi

#######################################

keymon.elf & # &> /mnt/sdcard/keymon.txt &
# ./batmon.sh &> /mnt/sdcard/batmon.txt &

#######################################

cd $(dirname "$0")
export EXEC_PATH=/tmp/minui_exec
touch "$EXEC_PATH" && sync
while [ -f "$EXEC_PATH" ]; do
	echo $CPU_SPEED_PERF > "$CPU_PATH"
	./minui.elf &> $LOGS_PATH/minui.txt
	sync
	
	NEXT="/tmp/next"
	if [ -f $NEXT ]; then
		CMD=`cat $NEXT`
		eval $CMD
		rm -f $NEXT
		echo $CPU_SPEED_PERF > "$CPU_PATH"
		sync
	fi
done
