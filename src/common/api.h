#ifndef __API_H__
#define __API_H__
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

///////////////////////////////

// TODO: tmp
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

uint32_t RGB_WHITE;
uint32_t RGB_BLACK;
uint32_t RGB_LIGHT_GRAY;
uint32_t RGB_DARK_GRAY;

enum {
	ASSET_WHITE_PILL,
	ASSET_BLACK_PILL,
	ASSET_DARK_GRAY_PILL,
	ASSET_BUTTON,
	ASSET_PAGE_BG,
	ASSET_STATE_BG,
	ASSET_PAGE,
	ASSET_BAR,
	ASSET_BAR_BG,
	ASSET_DOT,
	
	ASSET_COLORS,
	
	ASSET_BRIGHTNESS,
	ASSET_VOLUME_MUTE,
	ASSET_VOLUME,
	ASSET_BATTERY,
	ASSET_BATTERY_LOW,
	ASSET_BATTERY_FILL,
	ASSET_BATTERY_FILL_LOW,
	ASSET_BATTERY_BOLT,
};

typedef struct GFX_Fonts {
	TTF_Font* large; 	// menu
	TTF_Font* medium; 	// single char button label
	TTF_Font* small; 	// button hint
	TTF_Font* tiny; 	// multi char button label
} GFX_Fonts;
extern GFX_Fonts font;

SDL_Surface* GFX_init(void);
void GFX_clear(SDL_Surface* screen);
void GFX_clearAll(void);
void GFX_startFrame(void);
void GFX_flip(SDL_Surface* screen);
void GFX_quit(void);

// NOTE: all dimensions should be pre-scaled
void GFX_blitAsset(int asset, SDL_Rect* src_rect, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitPill(int asset, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitBattery(SDL_Surface* dst, SDL_Rect* dst_rect);
int GFX_getButtonWidth(char* hint, char* button);
void GFX_blitButton(char* hint, char*button, SDL_Surface* dst, SDL_Rect* dst_rect);

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

void POW_disablePowerOff(void);
void POW_powerOff(void);
void POW_fauxSleep(void);
int POW_preventAutosleep(void);
int POW_isCharging(void);
int POW_getBattery(void);

///////////////////////////////

#endif
