#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <errno.h>

#include <msettings.h>
#include "api.h"
#include "utils.h"
#include "defines.h"

///////////////////////////////

void LOG_note(int level, const char* fmt, ...) {
	char buf[1024] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	switch(level) {
#ifdef DEBUG
	case LOG_DEBUG:
		printf("DEBUG: %s", buf);
		break;
#endif
	case LOG_INFO:
		printf("INFO: %s", buf);
		break;
	case LOG_WARN:
		fprintf(stderr, "WARN: %s", buf);
		break;
	case LOG_ERROR:
		fprintf(stderr, "ERROR: %s", buf);
		break;
	default:
		break;
	}
	fflush(stdout);
}

///////////////////////////////

#define MAX_PRIVATE_DATA_SIZE 40
struct owlfb_disp_device{
    __u32 mType;
    __u32 mState;
    __u32 mPluginState;
    __u32 mWidth;
    __u32 mHeight;
    __u32 mRefreshRate;
    __u32 mWidthScale;
    __u32 mHeightScale;
    __u32 mCmdMode;
    __u32 mIcType;
    __u32 mPrivateInfo[MAX_PRIVATE_DATA_SIZE];
};

struct display_private_info
{
	int LCD_TYPE; // 1
	int	LCD_LIGHTENESS; // 128
	int LCD_SATURATION; // 7
	int LCD_CONSTRAST;  // 5
};

enum CmdMode
{
    SET_LIGHTENESS = 0,
    SET_SATURATION = 1,
    SET_CONSTRAST = 2,
	SET_DEFAULT = 3,

};

struct owlfb_sync_info {
	__u8 enabled;
	__u8 disp_id;
	__u16 reserved2;
};

struct owlfb_mem_info {
	__u32 size;
	__u8  type;
	__u8  reserved[3];
};

#define OWL_IOW(num, dtype)	_IOW('O', num, dtype)
#define OWLFB_WAITFORVSYNC            OWL_IOW(57,long long)
#define OWLFB_GET_DISPLAY_INFO		  OWL_IOW(74,struct owlfb_disp_device)
#define OWLFB_SET_DISPLAY_INFO		  OWL_IOW(75,struct owlfb_disp_device)
#define OWLFB_VSYNC_EVENT_EN	      OWL_IOW(67, struct owlfb_sync_info)

// not implemented?
#define OWLFB_SETUP_MEM	              OWL_IOW(55, struct owlfb_mem_info)
#define OWLFB_QUERY_MEM	              OWL_IOW(56, struct owlfb_mem_info)

///////////////////////////////

#define GFX_BUFFER_COUNT 3
#define GFX_ENABLE_VSYNC
#define GFX_ENABLE_BUFFER

///////////////////////////////

static struct GFX_Context {
	int fb;
	int pitch;
	int buffer;
	int buffer_size;
	int map_size;
	void* map;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	
	SDL_Surface* screen;
	SDL_Surface* assets;
} gfx;

static SDL_Rect asset_rects[] = {
	[ASSET_WHITE_PILL]		= (SDL_Rect){SCALE4( 1, 1,30,30)},
	[ASSET_BLACK_PILL]		= (SDL_Rect){SCALE4(33, 1,30,30)},
	[ASSET_DARK_GRAY_PILL]	= (SDL_Rect){SCALE4(65, 1,30,30)},
	[ASSET_BUTTON]			= (SDL_Rect){SCALE4( 1,33,20,20)},
	[ASSET_PAGE_BG]			= (SDL_Rect){SCALE4(64,33,15,15)},
	[ASSET_STATE_BG]		= (SDL_Rect){SCALE4(23,54, 8, 8)},
	[ASSET_PAGE]			= (SDL_Rect){SCALE4(39,54, 6, 6)},
	[ASSET_BAR]				= (SDL_Rect){SCALE4(33,58, 4, 4)},
	[ASSET_BAR_BG]			= (SDL_Rect){SCALE4(15,55, 4, 4)},
	[ASSET_DOT]				= (SDL_Rect){SCALE4(33,54, 2, 2)},
	
	[ASSET_BRIGHTNESS]		= (SDL_Rect){SCALE4(23,33,19,19)},
	[ASSET_VOLUME_MUTE]		= (SDL_Rect){SCALE4(44,33,10,16)},
	[ASSET_VOLUME]			= (SDL_Rect){SCALE4(44,33,18,16)},
	[ASSET_BATTERY]			= (SDL_Rect){SCALE4(47,51,17,10)},
	[ASSET_BATTERY_LOW]		= (SDL_Rect){SCALE4(66,51,17,10)},
	[ASSET_BATTERY_FILL]	= (SDL_Rect){SCALE4(81,33,12, 6)},
	[ASSET_BATTERY_FILL_LOW]= (SDL_Rect){SCALE4( 1,55,12, 6)},
	[ASSET_BATTERY_BOLT]	= (SDL_Rect){SCALE4(81,41,12, 6)},
};
static uint32_t asset_rgbs[ASSET_COLORS];
GFX_Fonts font;

///////////////////////////////

SDL_Surface* GFX_init(void) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(0);
	TTF_Init();
		
	// we're drawing to the (triple-buffered) framebuffer directly
	// but we still need to set video mode to initialize input events
	SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_HWSURFACE);
	
	// open framebuffer
	gfx.fb = open("/dev/fb0", O_RDWR);

	// configure framebuffer
	ioctl(gfx.fb, FBIOGET_VSCREENINFO, &gfx.vinfo);
	gfx.vinfo.bits_per_pixel = SCREEN_DEPTH;
	gfx.vinfo.xres = SCREEN_WIDTH;
	gfx.vinfo.yres = SCREEN_HEIGHT;
	gfx.vinfo.xres_virtual = SCREEN_WIDTH;
	gfx.vinfo.yres_virtual = SCREEN_HEIGHT;
#ifdef GFX_ENABLE_BUFFER
	gfx.vinfo.yres_virtual *= GFX_BUFFER_COUNT;
#endif
#if defined (GFX_ENABLE_BUFFER) && !defined (GFX_ENABLE_VSYNC) 
	gfx.vinfo.xoffset = 0;
	gfx.vinfo.yoffset = 0;
    gfx.vinfo.activate = FB_ACTIVATE_VBL;
#endif
    
	if (ioctl(gfx.fb, FBIOPUT_VSCREENINFO, &gfx.vinfo)) {
		printf("FBIOPUT_VSCREENINFO failed: %s (%i)\n", strerror(errno),  errno);
	}
	
	// get fixed screen info
   	ioctl(gfx.fb, FBIOGET_FSCREENINFO, &gfx.finfo);
	gfx.map_size = gfx.finfo.smem_len;
	gfx.map = mmap(0, gfx.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, gfx.fb, 0);
	memset(gfx.map, 0, gfx.map_size);
	
#ifdef GFX_ENABLE_VSYNC
	struct owlfb_sync_info sinfo;
	sinfo.enabled = 1;
	if (ioctl(gfx.fb, OWLFB_VSYNC_EVENT_EN, &sinfo)) {
		printf("OWLFB_VSYNC_EVENT_EN failed: %s (%i)\n", strerror(errno),  errno);
	}
#endif
	
	// buffer tracking
#ifdef GFX_ENABLE_BUFFER
	gfx.buffer = 1;
#else
	gfx.buffer = 0;
#endif
	gfx.buffer_size = SCREEN_PITCH * SCREEN_HEIGHT;

	// return screen
	gfx.screen = SDL_CreateRGBSurfaceFrom(gfx.map + gfx.buffer_size * gfx.buffer, SCREEN_WIDTH,SCREEN_HEIGHT, SCREEN_DEPTH,SCREEN_PITCH, 0,0,0,0);
	
	RGB_WHITE		= SDL_MapRGB(gfx.screen->format, TRIAD_WHITE);
	RGB_BLACK		= SDL_MapRGB(gfx.screen->format, TRIAD_BLACK);
	RGB_LIGHT_GRAY	= SDL_MapRGB(gfx.screen->format, TRIAD_LIGHT_GRAY);
	RGB_DARK_GRAY	= SDL_MapRGB(gfx.screen->format, TRIAD_DARK_GRAY);
	
	asset_rgbs[ASSET_WHITE_PILL]	= RGB_WHITE;
	asset_rgbs[ASSET_BLACK_PILL]	= RGB_BLACK;
	asset_rgbs[ASSET_DARK_GRAY_PILL]= RGB_DARK_GRAY;
	asset_rgbs[ASSET_BUTTON]		= RGB_WHITE;
	asset_rgbs[ASSET_PAGE_BG]		= RGB_WHITE;
	asset_rgbs[ASSET_STATE_BG]		= RGB_WHITE;
	asset_rgbs[ASSET_PAGE]			= RGB_BLACK;
	asset_rgbs[ASSET_BAR]			= RGB_WHITE;
	asset_rgbs[ASSET_BAR_BG]		= RGB_BLACK;
	asset_rgbs[ASSET_DOT]			= RGB_LIGHT_GRAY;
	
	char asset_path[MAX_PATH];
	sprintf(asset_path, RES_PATH "/assets@%ix.png", SCREEN_SCALE);
	gfx.assets = IMG_Load(asset_path);
	
	font.large 	= TTF_OpenFont(FONT_PATH, SCALE1(FONT_LARGE));
	font.medium = TTF_OpenFont(FONT_PATH, SCALE1(FONT_MEDIUM));
	font.small 	= TTF_OpenFont(FONT_PATH, SCALE1(FONT_SMALL));
	font.tiny 	= TTF_OpenFont(FONT_PATH, SCALE1(FONT_TINY));
	
	return gfx.screen;
}
void GFX_quit(void) {
	TTF_CloseFont(font.large);
	TTF_CloseFont(font.medium);
	TTF_CloseFont(font.small);
	TTF_CloseFont(font.tiny);
	
	SDL_FreeSurface(gfx.assets);
	GFX_clearAll();
	munmap(gfx.map, gfx.map_size);
	close(gfx.fb);
	SDL_Quit();
}

void GFX_clear(SDL_Surface* screen) {
	memset(screen->pixels, 0, gfx.buffer_size);
}
void GFX_clearAll(void) {
	memset(gfx.map, 0, gfx.map_size);
}

static int frame_start = 0;
void GFX_startFrame(void) {
	frame_start = SDL_GetTicks();
}
void GFX_flip(SDL_Surface* screen) {
	static int ticks = 0;
	ticks += 1;
#ifdef GFX_ENABLE_VSYNC
#define FRAME_BUDGET 17 // 60fps
	if (frame_start==0 || SDL_GetTicks()-frame_start<FRAME_BUDGET) { // only wait if we're under frame budget
		int arg = 1;
		ioctl(gfx.fb, OWLFB_WAITFORVSYNC, &arg);
	}
#endif

#ifdef GFX_ENABLE_BUFFER
	gfx.vinfo.yoffset = gfx.buffer * SCREEN_HEIGHT;
	ioctl(gfx.fb, FBIOPAN_DISPLAY, &gfx.vinfo);

	gfx.buffer += 1;
	if (gfx.buffer>=GFX_BUFFER_COUNT) gfx.buffer -= GFX_BUFFER_COUNT;
	screen->pixels = gfx.map + (gfx.buffer * gfx.buffer_size);
#endif
}

void GFX_blitAsset(int asset, SDL_Rect* src_rect, SDL_Surface* dst, SDL_Rect* dst_rect) {
	SDL_Rect* rect = &asset_rects[asset];
	SDL_Rect adj_rect = {
		.x = rect->x,
		.y = rect->y,
		.w = rect->w,
		.h = rect->h,
	};
	if (src_rect) {
		adj_rect.x += src_rect->x;
		adj_rect.y += src_rect->y;
		adj_rect.w  = src_rect->w;
		adj_rect.h  = src_rect->h;
	}
	SDL_BlitSurface(gfx.assets, &adj_rect, dst, dst_rect);
}
void GFX_blitPill(int asset, SDL_Surface* dst, SDL_Rect* dst_rect) {
	int x = dst_rect->x;
	int y = dst_rect->y;
	int w = dst_rect->w;
	int h = dst_rect->h;
	
	int r = h / 2;
	if (w < h) w = h;
	w -= h;
	
	GFX_blitAsset(asset, &(SDL_Rect){0,0,r,h}, dst, &(SDL_Rect){x,y});
	x += r;
	if (w>0) {
		SDL_FillRect(dst, &(SDL_Rect){x,y,w,h}, asset_rgbs[asset]);
		x += w;
	}
	GFX_blitAsset(asset, &(SDL_Rect){r,0,r,h}, dst, &(SDL_Rect){x,y});
}
void GFX_blitBattery(SDL_Surface* dst, SDL_Rect* dst_rect) {
	SDL_Rect rect = asset_rects[ASSET_BATTERY];
	int x = dst_rect->x;
	int y = dst_rect->y;
	x += (SCALE1(PILL_SIZE) - (rect.w + SCREEN_SCALE)) / 2;
	y += (SCALE1(PILL_SIZE) - rect.h) / 2;
	
	if (POW_isCharging()) {
		GFX_blitAsset(ASSET_BATTERY, NULL, dst, &(SDL_Rect){x,y});
		GFX_blitAsset(ASSET_BATTERY_BOLT, NULL, dst, &(SDL_Rect){x+SCALE1(3),y+SCALE1(2)});
	}
	else {
		int percent = POW_getBattery();
		GFX_blitAsset(percent<=10?ASSET_BATTERY_LOW:ASSET_BATTERY, NULL, dst, &(SDL_Rect){x,y});
		
		rect = asset_rects[ASSET_BATTERY_FILL];
		SDL_Rect clip = rect;
		clip.w *= percent;
		clip.w /= 100;
		if (clip.w<=0) return;
		clip.x = rect.w - clip.w;
		clip.y = 0;
		
		GFX_blitAsset(percent<=20?ASSET_BATTERY_FILL_LOW:ASSET_BATTERY_FILL, &clip, dst, &(SDL_Rect){x+SCALE1(3)+clip.x,y+SCALE1(2)});
	}
}
int GFX_getButtonWidth(char* hint, char* button) {
	int button_width = 0;
	int width;
	
	if (strlen(button)==1) {
		button_width += SCALE1(BUTTON_SIZE);
	}
	else {
		button_width += SCALE1(BUTTON_SIZE) / 2;
		TTF_SizeUTF8(font.tiny, button, &width, NULL);
		button_width += width;
	}
	button_width += SCALE1(BUTTON_MARGIN);
	
	TTF_SizeUTF8(font.small, hint, &width, NULL);
	button_width += width + SCALE1(BUTTON_MARGIN);
	return button_width;
}
void GFX_blitButton(char* hint, char*button, SDL_Surface* dst, SDL_Rect* dst_rect) {
	SDL_Surface* text;
	int ox = 0;
	
	// button
	if (strlen(button)==1) {
		GFX_blitAsset(ASSET_BUTTON, NULL, dst, dst_rect);

		// label
		text = TTF_RenderUTF8_Blended(font.medium, button, COLOR_BUTTON_TEXT);
		SDL_BlitSurface(text, NULL, dst, &(SDL_Rect){dst_rect->x+(SCALE1(BUTTON_SIZE)-text->w)/2,dst_rect->y+(SCALE1(BUTTON_SIZE)-text->h)/2});
		ox += SCALE1(BUTTON_SIZE);
		SDL_FreeSurface(text);
	}
	else {
		text = TTF_RenderUTF8_Blended(font.tiny, button, COLOR_BUTTON_TEXT);
		GFX_blitPill(ASSET_BUTTON, dst, &(SDL_Rect){dst_rect->x,dst_rect->y,SCALE1(BUTTON_SIZE)/2+text->w,SCALE1(BUTTON_SIZE)});
		ox += SCALE1(BUTTON_SIZE)/4;
		
		SDL_BlitSurface(text, NULL, dst, &(SDL_Rect){ox+dst_rect->x,dst_rect->y+(SCALE1(BUTTON_SIZE)-text->h)/2,text->w,text->h});
		ox += text->w;
		ox += SCALE1(BUTTON_SIZE)/4;
		SDL_FreeSurface(text);
	}
	
	ox += SCALE1(BUTTON_MARGIN);

	// hint text
	text = TTF_RenderUTF8_Blended(font.small, hint, COLOR_WHITE);
	SDL_BlitSurface(text, NULL, dst, &(SDL_Rect){ox+dst_rect->x,dst_rect->y+(SCALE1(BUTTON_SIZE)-text->h)/2,text->w,text->h});
	SDL_FreeSurface(text);
}

///////////////////////////////

// based on picoarch's audio 
// implementation, rewritten 
// to understand it better

#define MAX_SAMPLE_RATE 48000
#define BATCH_SIZE 100

typedef int (*SND_Resampler)(const SND_Frame frame);
static struct SND_Context {
	double frame_rate;
	
	int sample_rate_in;
	int sample_rate_out;
	
	int buffer_seconds;     // current_audio_buffer_size
	SND_Frame* buffer;	// buf
	size_t frame_count; 	// buf_len
	
	int frame_in;     // buf_w
	int frame_out;    // buf_r
	int frame_filled; // max_buf_w
	
	SND_Resampler resample;
} snd;
static void SND_audioCallback(void* userdata, uint8_t* stream, int len) { // plat_sound_callback
	if (snd.frame_count==0) return;
	
	int16_t *out = (int16_t *)stream;
	len /= (sizeof(int16_t) * 2);
	
	while (snd.frame_out!=snd.frame_in && len>0) {
		*out++ = snd.buffer[snd.frame_out].left;
		*out++ = snd.buffer[snd.frame_out].right;
		
		snd.frame_filled = snd.frame_out;
		
		snd.frame_out += 1;
		len -= 1;
		
		if (snd.frame_out>=snd.frame_count) snd.frame_out = 0;
	}
	
	while (len>0) {
		*out++ = 0;
		*out++ = 0;
		len -= 1;
	}
}
static void SND_resizeBuffer(void) { // plat_sound_resize_buffer
	snd.frame_count = snd.buffer_seconds * snd.sample_rate_in / snd.frame_rate;
	if (snd.frame_count==0) return;
	
	SDL_LockAudio();
	
	int buffer_bytes = snd.frame_count * sizeof(SND_Frame);
	snd.buffer = realloc(snd.buffer, buffer_bytes);
	
	memset(snd.buffer, 0, buffer_bytes);
	
	snd.frame_in = 0;
	snd.frame_out = 0;
	snd.frame_filled = snd.frame_count - 1;
	
	SDL_UnlockAudio();
}
static int SND_resampleNone(SND_Frame frame) { // audio_resample_passthrough
	snd.buffer[snd.frame_in++] = frame;
	if (snd.frame_in >= snd.frame_count) snd.frame_in = 0;
	return 1;
}
static int SND_resampleNear(SND_Frame frame) { // audio_resample_nearest
	static int diff = 0;
	int consumed = 0;

	if (diff < snd.sample_rate_out) {
		snd.buffer[snd.frame_in++] = frame;
		if (snd.frame_in >= snd.frame_count) snd.frame_in = 0;
		diff += snd.sample_rate_in;
	}

	if (diff >= snd.sample_rate_out) {
		consumed++;
		diff -= snd.sample_rate_out;
	}

	return consumed;
}
static void SND_selectResampler(void) { // plat_sound_select_resampler
	if (snd.sample_rate_in==snd.sample_rate_out) {
		snd.resample =  SND_resampleNone;
	}
	else {
		snd.resample = SND_resampleNear;
	}
}
size_t SND_batchSamples(const SND_Frame* frames, size_t frame_count) { // plat_sound_write / plat_sound_write_resample
	if (snd.frame_count==0) return 0;

	SDL_LockAudio();

	int consumed = 0;
	while (frame_count > 0) {
		int tries = 0;
		int amount = MIN(BATCH_SIZE, frame_count);

		while (tries < 10 && snd.frame_in==snd.frame_filled) {
			tries++;
			SDL_UnlockAudio();
			SDL_Delay(1);
			SDL_LockAudio();
		}

		while (amount && snd.frame_in != snd.frame_filled) {
			consumed = snd.resample(*frames);
			frames += consumed;
			amount -= consumed;
			frame_count -= consumed;
		}
	}
	SDL_UnlockAudio();
	
	return consumed;
}

void SND_init(double sample_rate, double frame_rate) { // plat_sound_init
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	
	snd.frame_rate = frame_rate;

	SDL_AudioSpec spec_in;
	SDL_AudioSpec spec_out;
	
	spec_in.freq = MIN(sample_rate, MAX_SAMPLE_RATE); // TODO: always MAX_SAMPLE_RATE on Miyoo Mini? use #ifdef PLATFORM_MIYOOMINI?
	spec_in.format = AUDIO_S16;
	spec_in.channels = 2;
	spec_in.samples = 512;
	spec_in.callback = SND_audioCallback;
	
	SDL_OpenAudio(&spec_in, &spec_out);
	
	snd.buffer_seconds = 5;
	snd.sample_rate_in  = sample_rate;
	snd.sample_rate_out = spec_out.freq;
	
	SND_selectResampler();
	SND_resizeBuffer();
	
	SDL_PauseAudio(0);
}
void SND_quit(void) { // plat_sound_finish
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	
	if (snd.buffer) {
		free(snd.buffer);
		snd.buffer = NULL;
	}
}

///////////////////////////////

static struct PAD_Context {
	int is_pressed;
	int just_pressed;
	int just_released;
} pad;
void PAD_reset(void) {
	pad.just_pressed = BTN_NONE;
	pad.is_pressed = BTN_NONE;
	pad.just_released = BTN_NONE;
}
void PAD_poll(void) {
	// reset transient state
	pad.just_pressed = BTN_NONE;
	pad.just_released = BTN_NONE;
	
	// the actual poll
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		int btn = BTN_NONE;
		if (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
			uint8_t code = event.key.keysym.scancode;
				 if (code==CODE_UP) 	btn = BTN_UP;
 			else if (code==CODE_DOWN)	btn = BTN_DOWN;
			else if (code==CODE_LEFT)	btn = BTN_LEFT;
			else if (code==CODE_RIGHT)	btn = BTN_RIGHT;
			else if (code==CODE_A)		btn = BTN_A;
			else if (code==CODE_B)		btn = BTN_B;
			else if (code==CODE_X)		btn = BTN_X;
			else if (code==CODE_Y)		btn = BTN_Y;
			else if (code==CODE_START)	btn = BTN_START;
			else if (code==CODE_SELECT)	btn = BTN_SELECT;
			else if (code==CODE_MENU)	btn = BTN_MENU;
			else if (code==CODE_L1)		btn = BTN_L1;
			else if (code==CODE_L2)		btn = BTN_L2;
			else if (code==CODE_R1)		btn = BTN_R1;
			else if (code==CODE_R2)		btn = BTN_R2;
			else if (code==CODE_VOL_UP)	btn = BTN_VOL_UP;
			else if (code==CODE_VOL_DN)	btn = BTN_VOL_DN;
			else if (code==CODE_POWER)	btn = BTN_POWER;
		}
		
		if (btn==BTN_NONE) continue;
		
		if (event.type==SDL_KEYUP) {
			pad.is_pressed &= ~btn; // unset
			pad.just_released |= btn; // set
		}
		else if ((pad.is_pressed & btn)==BTN_NONE) {
			pad.just_pressed |= btn; // set
			pad.is_pressed 	 |= btn; // set
		}
	}
}

// TODO: switch to macros? not if I want to move it to a separate file
int PAD_anyPressed(void)		{ return pad.is_pressed!=BTN_NONE; }
int PAD_justPressed(int btn)	{ return pad.just_pressed & btn; }
int PAD_isPressed(int btn)		{ return pad.is_pressed & btn; }
int PAD_justReleased(int btn)	{ return pad.just_released & btn; }

///////////////////////////////

static int can_poweroff = 1;
void POW_disablePowerOff(void) {
	can_poweroff = 0;
}

void POW_powerOff(void) {
	if (can_poweroff) {
		system("echo u > /proc/sysrq-trigger");
		system("echo s > /proc/sysrq-trigger");
		system("echo o > /proc/sysrq-trigger");
	}
}

#define BACKLIGHT_PATH "/sys/class/backlight/backlight.2/bl_power"
#define GOVERNOR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
static char governor[128];

static void POW_enterSleep(void) {
	SetRawVolume(0);
	putInt(BACKLIGHT_PATH, FB_BLANK_POWERDOWN);
	// save current governor (either ondemand or performance)
	getFile(GOVERNOR_PATH, governor, 128);
	trimTrailingNewlines(governor);

	putFile(GOVERNOR_PATH, "powersave");
	sync();
}
static void POW_exitSleep(void) {
	putInt(BACKLIGHT_PATH, FB_BLANK_UNBLANK);
	SetVolume(GetVolume());
	// restore previous governor
	putFile(GOVERNOR_PATH, governor);
}
static void POW_waitForWake(void) {
	SDL_Event event;
	int wake = 0;
	unsigned long sleep_ticks = SDL_GetTicks();
	while (!wake) {
		while (SDL_PollEvent(&event)) {
			if (event.type==SDL_KEYUP) {
				uint8_t code = event.key.keysym.scancode;
				if (code==CODE_POWER) {
					wake = 1;
					break;
				}
			}
		}
		SDL_Delay(200);
		if (can_poweroff && SDL_GetTicks()-sleep_ticks>=120000) { // increased to two minutes
			if (POW_isCharging()) sleep_ticks += 60000; // check again in a minute
			else POW_powerOff(); // TODO: not working...
		}
	}
	return;
}
void POW_fauxSleep(void) {
	GFX_clear(gfx.screen);
	PAD_reset();
	POW_enterSleep();
	system("killall -STOP keymon.elf");
	POW_waitForWake();
	system("killall -CONT keymon.elf");
	POW_exitSleep();
}
int POW_preventAutosleep(void) {
	return POW_isCharging();
}
int POW_isCharging(void) {
	return getInt("/sys/class/power_supply/battery/charger_online");
}
int POW_getBattery(void) {
	// return getInt("/sys/class/power_supply/battery/capacity"); // this is really inaccurate
	
	int i = getInt("/sys/class/power_supply/battery/voltage_now") / 10000; // ~320-420
	i = MIN(MAX(0, i-320), 100); // TODO: smooth this value before returning ala Mini

	// eh
	// if (i>80) return 100;
	// else if (i>60) return 80;
	// else if (i>40) return 60;
	// else if (i>20) return 40;
	// else if (i>10) return 20;
	// else if (i>5)  return 10;
	return i;
}