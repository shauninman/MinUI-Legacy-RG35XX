#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <errno.h>

#include "api.h"
#include "defines.h"

///////////////////////////////

// TODO: tmp
void powerOff(void) {
	system("echo u > /proc/sysrq-trigger");
	system("echo s > /proc/sysrq-trigger");
	system("echo o > /proc/sysrq-trigger");
}
void fauxSleep(void) { }
int preventAutosleep(void) { return 0; }
int isCharging() { return 0; }

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
} gfx;
SDL_Surface* GFX_init(void) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(0);
	TTF_Init();
	
	// char namebuf[MAX_PATH];
	// if (SDL_VideoDriverName(namebuf, MAX_PATH)) {
	// 	printf("SDL_VideoDriverName: %s\n", namebuf);
	// }
	
	// SDL_Rect **modes = SDL_ListModes(NULL, SDL_HWSURFACE);
	// if(modes == (SDL_Rect **)0){
	//   puts("No modes available!");
	//   exit(-1);
	// }
	//
	// if(modes == (SDL_Rect **)-1){
	//   puts("All resolutions available.");
	// }
	// else{
	// 	puts("Available Modes");
	//   	for(int i=0; modes[i]; ++i) {
	// 		printf("\t%d x %d\n", modes[i]->w, modes[i]->h);
	//   	}
	// }
	
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
	gfx.vinfo.yres_virtual = SCREEN_HEIGHT * SCREEN_BUFFER_COUNT;
	gfx.vinfo.xoffset = 0;
    gfx.vinfo.activate = FB_ACTIVATE_VBL;
    
	// gfx.vinfo.pixclock		= 0xb7bb;// 0xc350; // 0xc350=56 0xb7e0=59.94 0xb7bb=60
	// gfx.vinfo.left_margin	= 0x10;
	// gfx.vinfo.right_margin	= 0x14;
	// gfx.vinfo.upper_margin	= 0x0f;
	// gfx.vinfo.lower_margin	= 0x05;
	// gfx.vinfo.hsync_len		= 0x1e;
	// gfx.vinfo.vsync_len		= 0x02;
	
	ioctl(gfx.fb, FBIOPUT_VSCREENINFO, &gfx.vinfo);
	
	// printf("pixclock: 0x%04x\n", gfx.vinfo.pixclock);
	// printf("left_margin:  0x%02x\n", gfx.vinfo.left_margin);
	// printf("right_margin: 0x%02x\n", gfx.vinfo.right_margin);
	// printf("upper_margin: 0x%02x\n", gfx.vinfo.upper_margin);
	// printf("lower_margin: 0x%02x\n", gfx.vinfo.lower_margin);
	// printf("hsync_len: 0x%02x\n", gfx.vinfo.hsync_len);
	// printf("vsync_len: 0x%02x\n", gfx.vinfo.vsync_len);
	
	// get fixed screen info
   	ioctl(gfx.fb, FBIOGET_FSCREENINFO, &gfx.finfo);
	gfx.map_size = gfx.finfo.smem_len;
	gfx.map = mmap(0, gfx.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, gfx.fb, 0);
	
	// struct fb_vblank vblank;
	// ioctl(gfx.fb, FBIOGET_VBLANK, &vblank);
	// printf("flags: %i\n", vblank.flags);
	// printf("count: %i\n", vblank.count);
	// printf("vcount: %i\n", vblank.vcount);
	// printf("hcount: %i\n", vblank.hcount);
	
	// buffer tracking
	gfx.buffer = 0;
	gfx.buffer_size = SCREEN_PITCH * SCREEN_HEIGHT;
	
	// return screen
	gfx.screen = SDL_CreateRGBSurfaceFrom(gfx.map, SCREEN_WIDTH,SCREEN_HEIGHT, SCREEN_DEPTH,SCREEN_PITCH, 0,0,0,0);
	return gfx.screen;
}
void GFX_clear(SDL_Surface* screen) {
	memset(screen->pixels, 0, gfx.buffer_size);
}
void GFX_clearAll(void) {
	memset(gfx.map, 0, gfx.map_size);
}

// #define OWL_IOW(num, dtype)	_IOW('O', num, dtype)
// #define OWLFB_WAITFORVSYNC            OWL_IOW(57,long long)

void GFX_flip(SDL_Surface* screen) {
	// struct fb_vblank vblank;
	// ioctl(gfx.fb, FBIOGET_VBLANK, &vblank);
	// printf("flags: %i\n", vblank.flags);
	// printf("count: %i\n", vblank.count);
	// printf("vcount: %i\n", vblank.vcount);
	// printf("hcount: %i\n", vblank.hcount);
	
    // TODO: this would be moved to a thread
	// I'm not clear on why that would be necessary
	// if it's non-blocking and the pan will wait
	// until the next vblank...
	// what if the scaling was also moved to a thread?
	gfx.vinfo.yoffset = gfx.buffer * SCREEN_HEIGHT;
	int arg = 0;
	// ioctl(gfx.fb, OWLFB_WAITFORVSYNC, &arg); // TODO: this doesn't wait but it also doesn't error out like FBIO_WAITFORVSYNC...
	ioctl(gfx.fb, FBIOPAN_DISPLAY, &gfx.vinfo);
	
	gfx.buffer += 1;
	if (gfx.buffer>=SCREEN_BUFFER_COUNT) gfx.buffer -= SCREEN_BUFFER_COUNT;
	screen->pixels = gfx.map + (gfx.buffer * gfx.buffer_size);
}
void GFX_quit(void) {
	GFX_clearAll();
	munmap(gfx.map, gfx.map_size);
	close(gfx.fb);
	SDL_Quit();
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