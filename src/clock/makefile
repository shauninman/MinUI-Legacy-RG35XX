ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = clock.elf

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -marm -mtune=cortex-a9 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -fomit-frame-pointer
CFLAGS  += -I. -I../common -DPLATFORM=\"$(UNION_PLATFORM)\"
LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf -lmsettings -lpthread

all:
	$(CC) main.c ../common/utils.c ../common/api.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(TARGET)