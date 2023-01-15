#ifndef __API_H__
#define __API_H__
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

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
uint32_t RGB_GRAY;
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
	ASSET_UNDERLINE,
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
void GFX_blitABButtons(char* a, char* b, SDL_Surface* dst);
int GFX_blitButtonGroup(char** hints, SDL_Surface* dst, int align_right);

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
	BTN_ID_UP,
	BTN_ID_DOWN,
	BTN_ID_LEFT,
	BTN_ID_RIGHT,
	BTN_ID_A,
	BTN_ID_B,
	BTN_ID_X,
	BTN_ID_Y,
	BTN_ID_START,
	BTN_ID_SELECT,
	BTN_ID_L1,
	BTN_ID_R1,
	BTN_ID_L2,
	BTN_ID_R2,
	BTN_ID_MENU,
	BTN_ID_VOL_UP,
	BTN_ID_VOL_DN,
	BTN_ID_POWER,
	BTN_ID_COUNT,
};

enum {
	BTN_NONE	= 0,
	BTN_UP 		= 1 << BTN_ID_UP,
	BTN_DOWN	= 1 << BTN_ID_DOWN,
	BTN_LEFT	= 1 << BTN_ID_LEFT,
	BTN_RIGHT	= 1 << BTN_ID_RIGHT,
	BTN_A		= 1 << BTN_ID_A,
	BTN_B		= 1 << BTN_ID_B,
	BTN_X		= 1 << BTN_ID_X,
	BTN_Y		= 1 << BTN_ID_Y,
	BTN_START	= 1 << BTN_ID_START,
	BTN_SELECT	= 1 << BTN_ID_SELECT,
	BTN_L1		= 1 << BTN_ID_L1,
	BTN_R1		= 1 << BTN_ID_R1,
	BTN_L2		= 1 << BTN_ID_L2,
	BTN_R2		= 1 << BTN_ID_R2,
	BTN_MENU	= 1 << BTN_ID_MENU,
	BTN_VOL_UP	= 1 << BTN_ID_VOL_UP,
	BTN_VOL_DN	= 1 << BTN_ID_VOL_DN,
	BTN_POWER	= 1 << BTN_ID_POWER,
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
int PAD_justRepeated(int btn);

///////////////////////////////

void POW_disablePowerOff(void);
void POW_powerOff(void);
void POW_fauxSleep(void);
int POW_preventAutosleep(void);
int POW_isCharging(void);
int POW_getBattery(void);

///////////////////////////////

#endif
