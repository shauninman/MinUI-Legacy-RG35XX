ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = keymon.elf

CC = $(CROSS_COMPILE)gcc
CFLAGS	= -Os -lmsettings -lpthread -lrt -ldl -Wl,--gc-sections -s
CFLAGS  += -I. -I../common -DPLATFORM=\"$(UNION_PLATFORM)\"

all:
	$(CC) keymon.c -o $(TARGET) $(CFLAGS)
clean:
	rm -rf $(TARGET)