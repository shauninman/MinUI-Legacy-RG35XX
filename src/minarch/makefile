ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = minarch

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -marm -mtune=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=hard -march=armv7-a -fomit-frame-pointer
CFLAGS  += -I. -I../common -I./libretro-common/include -DPLATFORM=\"$(UNION_PLATFORM)\" -Ofast 
LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf -lmsettings -lpthread -lz
# CFLAGS  += -Wall -Wno-unused-variable -Wno-unused-function
# CFLAGS  += -fsanitize=address -fno-common
# LDFLAGS += -lasan

BUILD_DATE!=date +%Y.%m.%d
BUILD_HASH!=git rev-parse --short HEAD
CFLAGS += -DBUILD_DATE=\"${BUILD_DATE}\" -DBUILD_HASH=\"${BUILD_HASH}\"

all:
	$(CC) $(TARGET).c ../common/scaler_neon.c ../common/utils.c ../common/api.c -o $(TARGET).elf $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(TARGET).elf