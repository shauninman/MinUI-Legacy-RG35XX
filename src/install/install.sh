#!/system/bin/sh

# NOTE: this file is not chrooted so it's using stock's everything!

TF1_PATH=/mnt/mmc
TF2_PATH=/mnt/sdcard # TF1 is linked to this path if TF2 is missing
SYSTEM_PATH=/mnt/sdcard/.system/rg35xx

# old rootfs.img (alpha-only)
if [ -f $SYSTEM_PATH/rootfs.img ]; then
	rm $SYSTEM_PATH/rootfs.img
fi

if [ ! -f $FLAG_PATH ]; then
	BAK_PATH=$TF1_PATH/bak
	mkdir -p $BAK_PATH
	cp /misc/modules/gpio_keys_polled.ko $BAK_PATH
	cp /misc/boot_logo.bmp.gz $BAK_PATH
	cp /misc/kernel.dtb $BAK_PATH
	cp /misc/uImage $BAK_PATH
fi

was_updated() {
	for FILE in /misc/* /misc/*/*; do
		A_PATH=$FILE
		B_PATH=$SYSTEM_PATH/dat/$(busybox basename "$A_PATH")
	
		if [ ! -f "$B_PATH" ]; then
			continue
		fi
	
		A_SUM=$(busybox md5sum $A_PATH | busybox cut -d ' ' -f 1)
		B_SUM=$(busybox md5sum $B_PATH | busybox cut -d ' ' -f 1)
	
		if [[ "$A_SUM" != "$B_SUM" ]]; then
			return 0
		fi
	done
	
	return 1
}

if [ ! -f $FLAG_PATH ] || was_updated; then
	echo "updating misc partition"
	mount -o remount,rw /dev/block/actb /misc
	rm -f /misc/uImage
	cp $SYSTEM_PATH/dat/uImage /misc
	cp $SYSTEM_PATH/dat/dmenu.bin /misc
	if [ ! -f $FLAG_PATH ]; then
		# only replace boot logo on install not update!
		cp $SYSTEM_PATH/dat/boot_logo.bmp.gz /misc
	fi
	cp $SYSTEM_PATH/dat/kernel.dtb /misc
	cp $SYSTEM_PATH/dat/gpio_keys_polled.ko /misc/modules
	touch $FLAG_PATH
	sync && reboot
fi