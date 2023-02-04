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

all: lib sys cores tools dtb bundle readmes zip report
	
lib:
	cd ./src/libmsettings && make

sys:
	cd ./src/keymon && make
	cd ./src/minarch && make
	cd ./src/minui && make

cores:
	echo "TODO: cores"

tools:
	cd ./src/clock && make
	cd ./third-party/DinguxCommander && make -j

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
	
	# populate system
	cp ~/buildroot/output/images/rootfs.ext2 ./build/SYSTEM/rg35xx/
	cp ./src/dts/kernel.dtb ./build/SYSTEM/rg35xx/dat
	cp ./src/libmsettings/libmsettings.so ./build/SYSTEM/rg35xx/lib
	cp ./src/keymon/keymon.elf ./build/SYSTEM/rg35xx/bin
	cp ./src/minarch/minarch.elf ./build/SYSTEM/rg35xx/bin
	cp ./src/minui/minui.elf ./build/SYSTEM/rg35xx/paks/MinUI.pak
	cp ./src/clock/clock.elf ./build/EXTRAS/Tools/rg35xx/Clock.pak
	cp ./third-party/DinguxCommander/output/DinguxCommander ./build/EXTRAS/Tools/rg35xx/Files.pak
	cp -R ./third-party/DinguxCommander/res ./build/EXTRAS/Tools/rg35xx/Files.pak/
	
	mkdir -p ./build/PAYLOAD
	mv ./build/SYSTEM ./build/PAYLOAD/.system
	
	# TODO: move to zip target
	cd ./build/PAYLOAD && find . -type f -name '.DS_Store' -delete # TODO: do this before echo zip
	cd ./build/PAYLOAD && zip -r MinUI.zip .system
	mv ./build/PAYLOAD/MinUI.zip ./build/BASE
	
readmes:
	# TODO
	echo

zip:
	# TODO: 
	echo

report:
	echo "finished building r${RELEASE_TIME}-${RELEASE_DOT}"

clean:
	rm -rf ./build
	cd ./src/libmsettings && make clean
	cd ./src/keymon && make clean
	cd ./src/minui && make clean
	cd ./src/minarch && make clean
	echo "TODO: clean cores"
	cd ./src/clock && make clean
	cd ./third-party/DinguxCommander && make clean
	