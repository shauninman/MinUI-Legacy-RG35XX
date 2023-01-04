#ifndef __API_H__
#define __API_H__
#include <SDL/SDL.h>

///////////////////////////////

// TODO: tmp
void powerOff(void);
void fauxSleep(void);
int preventAutosleep(void);
int isCharging();
#define PAD_justRepeated PAD_justPressed

///////////////////////////////

enum {
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
};

#define LOG_debug(fmt, ...) LOG_note(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_info(fmt, ...) LOG_note(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_warn(fmt, ...) LOG_note(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_error(fmt, ...) LOG_note(LOG_ERROR, fmt, ##__VA_ARGS__)
void LOG_note(int level, const char* fmt, ...);

///////////////////////////////

SDL_Surface* GFX_init(void);
void GFX_clear(SDL_Surface* screen);
void GFX_clearAll(void);
void GFX_flip(SDL_Surface* screen);
void GFX_quit(void);

///////////////////////////////

typedef struct SND_Frame {
	int16_t left;
	int16_t right;
} SND_Frame;

void SND_init(double sample_rate, double frame_rate);
size_t SND_batchSamples(const SND_Frame* frames, size_t frame_count);
void SND_quit(void);

///////////////////////////////

enum {
	BTN_NONE	= 0,
	BTN_UP 		= 1 << 0,
	BTN_DOWN	= 1 << 1,
	BTN_LEFT	= 1 << 2,
	BTN_RIGHT	= 1 << 3,
	BTN_A		= 1 << 4,
	BTN_B		= 1 << 5,
	BTN_X		= 1 << 6,
	BTN_Y		= 1 << 7,
	BTN_START	= 1 << 8,
	BTN_SELECT	= 1 << 9,
	BTN_L1		= 1 << 10,
	BTN_R1		= 1 << 11,
	BTN_L2		= 1 << 12,
	BTN_R2		= 1 << 13,
	BTN_MENU	= 1 << 14,
	BTN_VOL_UP	= 1 << 15,
	BTN_VOL_DN	= 1 << 16,
	BTN_POWER	= 1 << 17,
};

// TODO: this belongs in defines.h or better yet a platform.h
#define BTN_RESUME BTN_X
#define BTN_SLEEP BTN_POWER

void PAD_reset(void);
void PAD_poll(void);
int PAD_anyPressed(void);
int PAD_justPressed(int btn);
int PAD_isPressed(int btn);
int PAD_justReleased(int btn);

///////////////////////////////

#endif
