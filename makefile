.PHONY: clean

###########################################################

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
$(error please specify PLATFORM, eg. PLATFORM=trimui make)
endif

###########################################################

BUILD_HASH!=git rev-parse --short HEAD

RELEASE_TIME!=date +%Y%m%d
RELEASE_BASE=MinUI-$(RELEASE_TIME)
RELEASE_DOT!=find ./releases/. -regex ".*/$(RELEASE_BASE)-[0-9]+-base\.zip" -printf '.' | wc -m
RELEASE_NAME=$(RELEASE_BASE)-$(RELEASE_DOT)

# TODO: this needs to consider the different platforms, eg. rootfs.ext2 should only be copied in rg35xx-toolchain

all: lib sys all-cores tools dtb bundle readmes zip report

lib:
	cd ./src/libmsettings && make

sys:
	cd ./src/keymon && make
	cd ./src/minarch && make
	cd ./src/minui && make
	cd ./src/boot && ./build.sh

all-cores:
	cd ./cores && make

tools:
	cd ./src/clock && make
	cd ./other/DinguxCommander && make -j

dtb:
	cd ./src/dts/ && make

bundle:
	# ready build
	rm -rf ./build
	mkdir -p ./releases
	cp -R ./skeleton ./build
	
	# remove authoring detritus
	cd ./build && find . -type f -name '.keep' -delete
	cd ./build && find . -type f -name '*.meta' -delete
	
	cp ./src/boot/output/dmenu.bin ./build/BASE
	cp ./src/boot/output/dmenu.bin ./build/SYSTEM/rg35xx/dat
	cp ./src/install/install.sh ./build/SYSTEM/rg35xx/bin
	echo "$(BUILD_HASH)" > ./build/SYSTEM/commit.txt
	echo "$(RELEASE_NAME)" > ./build/SYSTEM/release.txt
	
	# prepare boot logo
	cd ./build/SYSTEM/rg35xx/dat && convert boot_logo.png -type truecolor boot_logo.bmp && rm boot_logo.png && gzip boot_logo.bmp

	# populate system
	cp ~/buildroot/output/images/rootfs.ext2 ./build/SYSTEM/rg35xx
	cp ./src/dts/kernel.dtb ./build/SYSTEM/rg35xx/dat
	cp ./src/libmsettings/libmsettings.so ./build/SYSTEM/rg35xx/lib
	cp ./src/keymon/keymon.elf ./build/SYSTEM/rg35xx/bin
	cp ./src/minarch/minarch.elf ./build/SYSTEM/rg35xx/bin
	cp ./src/minui/minui.elf ./build/SYSTEM/rg35xx/paks/MinUI.pak
	cp ./src/clock/clock.elf ./build/EXTRAS/Tools/rg35xx/Clock.pak
	
	# stock cores
	cp ./cores/output/fceumm_libretro.so ./build/SYSTEM/rg35xx/cores
	cp ./cores/output/gambatte_libretro.so ./build/SYSTEM/rg35xx/cores
	cp ./cores/output/gpsp_libretro.so ./build/SYSTEM/rg35xx/cores
	cp ./cores/output/pcsx_rearmed_libretro.so ./build/SYSTEM/rg35xx/cores
	cp ./cores/output/picodrive_libretro.so ./build/SYSTEM/rg35xx/cores
	cp ./cores/output/snes9x2005_plus_libretro.so ./build/SYSTEM/rg35xx/cores

	# extras
	cp ./cores/output/mednafen_vb_libretro.so ./build/EXTRAS/Emus/rg35xx/VB.pak
	cp ./cores/output/pokemini_libretro.so ./build/EXTRAS/Emus/rg35xx/PKM.pak
	cp ./cores/output/fake08_libretro.so ./build/EXTRAS/Emus/rg35xx/P8.pak
	cp ./other/DinguxCommander/output/DinguxCommander ./build/EXTRAS/Tools/rg35xx/Files.pak
	cp -R ./other/DinguxCommander/res ./build/EXTRAS/Tools/rg35xx/Files.pak/
	
readmes:
	fmt -w 40 -s ./skeleton/BASE/README.txt > ./build/BASE/README.txt
	fmt -w 40 -s ./skeleton/EXTRAS/README.txt > ./build/EXTRAS/README.txt

zip:
	cd ./build && find . -type f -name '.DS_Store' -delete
	mkdir -p ./build/PAYLOAD
	mv ./build/SYSTEM ./build/PAYLOAD/.system	
	
	cd ./build/PAYLOAD && zip -r MinUI.zip .system
	mv ./build/PAYLOAD/MinUI.zip ./build/BASE
	
	cd ./build/BASE && zip -r ../../releases/$(RELEASE_NAME)-base.zip Bios Roms Saves dmenu.bin MinUI.zip README.txt
	cd ./build/EXTRAS && zip -r ../../releases/$(RELEASE_NAME)-extras.zip Bios Emus Roms Saves Tools README.txt
	echo "$(RELEASE_NAME)" > ./build/latest.txt

report:
	echo "finished building r${RELEASE_TIME}-${RELEASE_DOT}"

clean:
	rm -rf ./build
	cd ./src/libmsettings && make clean
	cd ./src/keymon && make clean
	cd ./src/minui && make clean
	cd ./src/minarch && make clean
	cd ./src/boot && rm -rf ./output
	cd ./cores && make clean
	cd ./src/clock && make clean
	cd ./other/DinguxCommander && make clean
	