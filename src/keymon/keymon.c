// miyoomini/keymon.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>

#include <msettings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "defines.h"

//	for ev.value
#define RELEASED	0
#define PRESSED		1
#define REPEAT		2

#define INPUT_COUNT 2
static int inputs[INPUT_COUNT];
static struct input_event ev;
static int jack_fd;
static pthread_t jack_pt;

// TODO: HDMI?

#define JACK_STATE_PATH "/sys/class/switch/h2w/state"
#define HDMI_STATE_PATH "/sys/class/switch/hdmi/state"

static void* watchJack(void *arg) {
	uint32_t has_headphones;
	uint32_t had_headphones;
	
	FILE *file = fopen(JACK_STATE_PATH, "r");
	fscanf(file, "%i", &has_headphones);
	had_headphones = has_headphones;
	SetJack(has_headphones);
	
	while(1) {
		sleep(1);
		rewind(file);
		fscanf(file, "%i", &has_headphones);
		if (had_headphones!=has_headphones) {
			had_headphones = has_headphones;
			SetJack(has_headphones);
		}
	}
	return 0;
}

int main (int argc, char *argv[]) {
	InitSettings();
	pthread_create(&jack_pt, NULL, &watchJack, NULL);
	
	char path[32];
	for (int i=0; i<INPUT_COUNT; i++) {
		sprintf(path, "/dev/input/event%i", i);
		inputs[i] = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	}
	
	register uint32_t input;
	register uint32_t val;
	register uint32_t menu_pressed = 0;
	register uint32_t power_pressed = 0;
	uint32_t repeat_volume = 0;
	
	// TODO: enable key repeat (not supported natively)
	while (1) {
		for (int i=0; i<INPUT_COUNT; i++) {
			input = inputs[i];
			while(read(input, &ev, sizeof(ev))==sizeof(ev)) {
				val = ev.value;
				if (( ev.type != EV_KEY ) || ( val > REPEAT )) continue;
				switch (ev.code) {
					case CODE_MENU:
						if ( val != REPEAT ) menu_pressed = val;
						break;
					case CODE_POWER:
						if ( val != REPEAT ) power_pressed = val;
						break;
					case CODE_VOL_DN:
						if ( val == REPEAT ) {
							// Adjust repeat speed to 1/2
							val = repeat_volume;
							repeat_volume ^= PRESSED;
						} else {
							repeat_volume = 0;
						}
						if ( val == PRESSED ) {
							if (menu_pressed) {
								val = GetBrightness();
								if (val>BRIGHTNESS_MIN) SetBrightness(--val);
							}
							else {
								val = GetVolume();
								if (val>VOLUME_MIN) SetVolume(--val);
							}
						}
						break;
					case CODE_VOL_UP:
						if ( val == REPEAT ) {
							// Adjust repeat speed to 1/2
							val = repeat_volume;
							repeat_volume ^= PRESSED;
						} else {
							repeat_volume = 0;
						}
						if ( val == PRESSED ) {
							if (menu_pressed) {
								val = GetBrightness();
								if (val<BRIGHTNESS_MAX) SetBrightness(++val);
							}
							else {
								val = GetVolume();
								if (val<VOLUME_MAX) SetVolume(++val);
							}
						}
						break;
					default:
						break;
				}
			}
		}
		usleep(16666); // 60fps
	}
}
