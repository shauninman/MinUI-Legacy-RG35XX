#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>

#include "msettings.h"

///////////////////////////////////////

typedef struct Settings {
	int version; // future proofing
	int brightness;
	int headphones;
	int speaker;
	int unused[3]; // for future use
	int jack; // NOTE: doesn't really need to be persisted but still needs to be shared
} Settings;
static Settings DefaultSettings = {
	.version = 1,
	.brightness = 2,
	.headphones = 4,
	.speaker = 8,
	.jack = 0,
};
static Settings* settings;

#define SHM_KEY "/SharedSettings"
// static char SettingsPath[256];
static char* SettingsPath = "/mnt/sdcard/.userdata/rg35xx/msettings.bin";
static int shm_fd = -1;
static int is_host = 0;
static int shm_size = sizeof(Settings);

void InitSettings(void) {
	// sprintf(SettingsPath, "%s/msettings.bin", getenv("USERDATA_PATH"));
	
	shm_fd = shm_open(SHM_KEY, O_RDWR | O_CREAT | O_EXCL, 0644); // see if it exists
	if (shm_fd==-1 && errno==EEXIST) { // already exists
		puts("Settings client");
		shm_fd = shm_open(SHM_KEY, O_RDWR, 0644);
		settings = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	}
	else { // host
		puts("Settings host");
		is_host = 1;
		// we created it so set initial size and populate
		ftruncate(shm_fd, shm_size);
		settings = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		
		int fd = open(SettingsPath, O_RDONLY);
		if (fd>=0) {
			read(fd, settings, shm_size);
			// TODO: use settings->version for future proofing
			close(fd);
		}
		else {
			// load defaults
			memcpy(settings, &DefaultSettings, shm_size);
		}
	}
	printf("brightness: %i\nspeaker: %i \n", settings->brightness, settings->speaker);

	SetVolume(GetVolume());
	SetBrightness(GetBrightness());
}
void QuitSettings(void) {
	munmap(settings, shm_size);
	if (is_host) shm_unlink(SHM_KEY);
}
static inline void SaveSettings(void) {
	int fd = open(SettingsPath, O_CREAT|O_WRONLY, 0644);
	if (fd>=0) {
		write(fd, settings, shm_size);
		close(fd);
		sync();
	}
}

int GetBrightness(void) { // 0-10
	return settings->brightness;
}
void SetBrightness(int value) {
	int raw;
	switch (value) {
		case 0: raw=8; break;
		case 1: raw=16; break;
		case 2: raw=32; break;
		case 3: raw=64; break;
		case 4: raw=128; break;
		case 5: raw=192; break;
		case 6: raw=256; break;
		case 7: raw=384; break;
		case 8: raw=512; break;
		case 9: raw=768; break;
		case 10: raw=1024; break;
	}
	SetRawBrightness(raw);
	settings->brightness = value;
	SaveSettings();
}

int GetVolume(void) { // 0-20
	return settings->jack ? settings->headphones : settings->speaker;
}
void SetVolume(int value) {
	if (settings->jack) settings->headphones = value;
	else settings->speaker = value;
	
	int raw = value * 2;
	SetRawVolume(raw);
	SaveSettings();
}

void SetRawBrightness(int val) { // 0 - 1024
	int fd = open("/sys/class/backlight/backlight.2/brightness", O_WRONLY);
	if (fd>=0) {
		dprintf(fd,"%d",val);
		close(fd);
	}
}
void SetRawVolume(int val) { // 0 - 40
	int fd = open("/sys/class/volume/value", O_WRONLY);
	if (fd>=0) {
		dprintf(fd,"%d",val);
		close(fd);
	}
}

int GetJack(void) {
	// return /sys/class/switch/h2w/state==1`
	// access("/dev/dsp1", F_OK)==0
	return settings->jack;
}
void SetJack(int value) { // monitored and set by thread in keymon
	settings->jack = value;
	SetVolume(GetVolume());
}