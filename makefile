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

all: lib sys cores tools dtb payload readmes zip report
	
lib:
	cd ./src/libmsettings && make

sys:
	cd ./src/keymon && make
	cd ./src/minui && make
	cd ./src/minarch && make

cores:
	echo "TODO: cores"

tools:
	cd ./src/clock && make
	cd ./third-party/DinguxCommander && make -j

dtb:
	cd ./src/dts/ && make

payload:
	rm -rf ./build
	mkdir -p ./releases
	mkdir -p ./build
	# cp ~/buildroot/output/images/rootfs.ext2 ./build/rootfs.img
	
	echo "TODO: payload"
	echo "TDOO: remove .keep and *.meta files"
	

readmes:
	echo "TODO: readmes"

zip:
	cd ./build && find . -type f -name '.keep' -delete
	cd ./build && find . -type f -name '*.meta' -delete
	cd ./build && find . -type f -name '.DS_Store' -delete
	echo "TODO: zip"

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
	