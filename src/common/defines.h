#ifndef __DEFINES_H__
#define __DEFINES_H__

#define CODE_UP			0x5A
#define CODE_DOWN		0x5B
#define CODE_LEFT		0x5C
#define CODE_RIGHT		0x5D
#define CODE_A			0x5E
#define CODE_B			0x5F
#define CODE_X			0x60
#define CODE_Y			0x61
#define CODE_START		0x62
#define CODE_SELECT		0x63
#define CODE_L1			0x64
#define CODE_R1			0x65
#define CODE_L2			0x66
#define CODE_R2			0x67
#define CODE_MENU		0x68
#define CODE_VOL_UP		0x6C
#define CODE_VOL_DN		0x6D
#define CODE_POWER		0x74

#define VOLUME_MIN 		0
#define VOLUME_MAX 		20
#define BRIGHTNESS_MIN 	0
#define BRIGHTNESS_MAX 	10

#define MAX_PATH 512

#define SDCARD_PATH "/mnt/sdcard"
#define ROMS_PATH SDCARD_PATH "/Roms"
#define SYSTEM_PATH SDCARD_PATH "/.system/" PLATFORM
#define RES_PATH SDCARD_PATH "/.system/res"
#define FONT_PATH RES_PATH "/BPreplayBold-unhinted.otf"
#define USERDATA_PATH SDCARD_PATH "/.userdata/" PLATFORM
#define PAKS_PATH SYSTEM_PATH "/paks"
#define RECENT_PATH USERDATA_PATH "/.minui/recent.txt"
#define FAUX_RECENT_PATH SDCARD_PATH "/Recently Played"
#define COLLECTIONS_PATH SDCARD_PATH "/Collections"

#define LAST_PATH "/tmp/last.txt" // transient
#define CHANGE_DISC_PATH "/tmp/change_disc.txt"
#define RESUME_SLOT_PATH "/tmp/mmenu_slot.txt"
#define AUTO_RESUME_PATH USERDATA_PATH "/.miniui/auto_resume.txt"
#define AUTO_RESUME_SLOT "9"
#define ENABLE_SIMPLE_MODE_PATH USERDATA_PATH "/enable-simple-mode"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// SNES (stretched to 4:3)
// #define SCREEN_WIDTH 1024
// #define SCREEN_HEIGHT 896

// GBA 
// #define SCREEN_WIDTH 960
// #define SCREEN_HEIGHT 720

// GB converted to 4:3 with full height
// #define SCREEN_WIDTH 768
// #define SCREEN_HEIGHT 576

#define SCREEN_DEPTH 16
#define SCREEN_BPP 2
#define SCREEN_PITCH SCREEN_WIDTH * SCREEN_BPP

#define MAIN_ROW_COUNT 7

///////////////////////////////

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

#endif