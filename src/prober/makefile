ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = Probe.pak/prober.elf

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -marm -mtune=cortex-a9 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7-a -fomit-frame-pointer
CFLAGS  += -I.
# LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf

all:
	$(CC) prober.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(TARGET)