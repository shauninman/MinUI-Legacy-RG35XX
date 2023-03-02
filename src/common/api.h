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

#define FIXED_WIDTH 640
#define FIXED_HEIGHT 480
#define FIXED_BPP 2
#define FIXED_DEPTH FIXED_BPP * 8
#define FIXED_PITCH FIXED_WIDTH * FIXED_BPP
#define FIXED_SIZE FIXED_HEIGHT * FIXED_PITCH

#define HDMI_WIDTH 1280
#define HDMI_HEIGHT 720
#define HDMI_PITCH HDMI_WIDTH * FIXED_BPP
#define HDMI_SIZE HDMI_HEIGHT * HDMI_PITCH

#define PAGE_COUNT 2
#define PAGE_SCALE 2
#define PAGE_WIDTH FIXED_WIDTH * PAGE_SCALE
#define PAGE_HEIGHT FIXED_HEIGHT * PAGE_SCALE
#define PAGE_PITCH PAGE_WIDTH * FIXED_BPP
#define PAGE_SIZE PAGE_HEIGHT * PAGE_PITCH

#define VIRTUAL_WIDTH PAGE_WIDTH
#define VIRTUAL_HEIGHT PAGE_HEIGHT * PAGE_COUNT
#define VIRTUAL_PITCH PAGE_WIDTH * FIXED_BPP
#define VIRTUAL_SIZE VIRTUAL_HEIGHT * VIRTUAL_PITCH

///////////////////////////////

extern uint32_t RGB_WHITE;
extern uint32_t RGB_BLACK;
extern uint32_t RGB_LIGHT_GRAY;
extern uint32_t RGB_GRAY;
extern uint32_t RGB_DARK_GRAY;

enum {
	ASSET_WHITE_PILL,
	ASSET_BLACK_PILL,
	ASSET_DARK_GRAY_PILL,
	ASSET_OPTION,
	ASSET_BUTTON,
	ASSET_PAGE_BG,
	ASSET_STATE_BG,
	ASSET_PAGE,
	ASSET_BAR,
	ASSET_BAR_BG,
	ASSET_BAR_BG_MENU,
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
	
	ASSET_SCROLL_UP,
	ASSET_SCROLL_DOWN,
};

typedef struct GFX_Fonts {
	TTF_Font* large; 	// menu
	TTF_Font* medium; 	// single char button label
	TTF_Font* small; 	// button hint
	TTF_Font* tiny; 	// multi char button label
} GFX_Fonts;
extern GFX_Fonts font;

enum {
	MODE_MAIN,
	MODE_MENU,
};

SDL_Surface* GFX_init(int mode);
SDL_Surface* GFX_resize(int width, int height, int pitch);
void GFX_setMode(int mode);
void GFX_clear(SDL_Surface* screen);
void GFX_clearAll(void);
void GFX_startFrame(void);
void GFX_flip(SDL_Surface* screen);
void GFX_sync(void); // call this to maintain 60fps when not calling GFX_flip() this frame
void GFX_quit(void);

enum {
	VSYNC_OFF = 0,
	VSYNC_LENIENT, // default
	VSYNC_STRICT,
};

int GFX_getVsync(void);
void GFX_setVsync(int vsync);

SDL_Surface* GFX_getBufferCopy(void); // must be freed by caller
int GFX_truncateText(TTF_Font* font, const char* in_name, char* out_name, int max_width); // returns final width (including pill padding)
int GFX_wrapText(TTF_Font* font, char* str, int max_width, int max_lines);

// NOTE: all dimensions should be pre-scaled
void GFX_blitAsset(int asset, SDL_Rect* src_rect, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitPill(int asset, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitRect(int asset, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitBattery(SDL_Surface* dst, SDL_Rect* dst_rect);
int GFX_getButtonWidth(char* hint, char* button);
void GFX_blitButton(char* hint, char*button, SDL_Surface* dst, SDL_Rect* dst_rect);
void GFX_blitMessage(char* msg, SDL_Surface* dst, SDL_Rect* dst_rect);

int GFX_blitHardwareGroup(SDL_Surface* dst, int show_setting);
int GFX_blitButtonGroup(char** hints, SDL_Surface* dst, int align_right);

void GFX_sizeText(TTF_Font* font, char* str, int leading, int* w, int* h);
void GFX_blitText(TTF_Font* font, char* str, int leading, SDL_Color color, SDL_Surface* dst, SDL_Rect* dst_rect);

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
	BTN_ID_NONE = -1,
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
	BTN_ID_PLUS,
	BTN_ID_MINUS,
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
	BTN_PLUS	= 1 << BTN_ID_PLUS,
	BTN_MINUS	= 1 << BTN_ID_MINUS,
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

int PAD_tappedMenu(uint32_t now); // special case, returns 1 on release of BTN_MENU within 250ms and BTN_PLUS/BTN_MINUS haven't been pressed

///////////////////////////////

void VIB_init(void);
void VIB_quit(void);
void VIB_setStrength(int strength);
 int VIB_getStrength(void);
	
///////////////////////////////

#define BRIGHTNESS_BUTTON_LABEL "+ -" // ew
typedef void (*POW_callback_t)(void);
void POW_init(void);
void POW_quit(void);
void POW_warn(int enable);

void POW_update(int* dirty, int* show_setting, POW_callback_t before_sleep, POW_callback_t after_sleep);

void POW_disablePowerOff(void);
void POW_powerOff(void);

void POW_fauxSleep(void);
void POW_disableAutosleep(void);
void POW_enableAutosleep(void);
int POW_preventAutosleep(void);

int POW_isCharging(void);
int POW_getBattery(void);

#define CPU_SPEED_MENU			 504000 // 240000 had latency issues
#define CPU_SPEED_POWERSAVE 	1104000
#define CPU_SPEED_NORMAL 		1296000
#define CPU_SPEED_PERFORMANCE	1488000
void POW_setCPUSpeed(int speed);

///////////////////////////////

#endif
