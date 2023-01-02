ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = minarch.elf

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -marm -mtune=cortex-a9 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -fomit-frame-pointer
CFLAGS  += -I. -I../common -I./libretro-common/include -DPLATFORM=\"$(UNION_PLATFORM)\" -Ofast
LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf -lmsettings

all:
	$(CC) main.c ../common/scaler_neon.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(TARGET)