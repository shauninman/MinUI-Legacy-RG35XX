ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = minarch.elf

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -marm -mtune=cortex-a9 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -fomit-frame-pointer
CFLAGS  += -I. -I../common -I./libretro-common/include -DPLATFORM=\"$(UNION_PLATFORM)\" -Ofast # -Wall -Wno-unused-variable -Wno-unused-function
LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf -lmsettings -lpthread

all:
	$(CC) main.c ../common/scaler_neon.c ../common/utils.c ../common/api.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(TARGET)