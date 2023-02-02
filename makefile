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

all: lib sys cores tools payload readmes zip report
	
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

payload:
	rm -rf ./build
	mkdir -p ./releases
	mkdir -p ./build
	
	echo "TODO: payload"

readmes:
	echo "TODO: readmes"

zip:
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
	