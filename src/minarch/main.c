#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <libgen.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "libretro.h"
#include "defines.h"
#include "scaler_neon.h"

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
	
	// we're drawing to the (triple-buffered) framebuffer directly
	// but we still need to set video mode to initialize input events
	SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, SDL_SWSURFACE);
	SDL_ShowCursor(0);
	
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
    ioctl(gfx.fb, FBIOPUT_VSCREENINFO, &gfx.vinfo);
	
	// get fixed screen info
   	ioctl(gfx.fb, FBIOGET_FSCREENINFO, &gfx.finfo);
	gfx.map_size = gfx.finfo.smem_len;
	gfx.map = mmap(0, gfx.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, gfx.fb, 0);
	
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
void GFX_flip(SDL_Surface* screen) {
    // TODO: this would be moved to a thread
	// I'm not clear on why that would be necessary
	// if it's non-blocking and the pan will wait
	// until the next vblank...
	gfx.vinfo.yoffset = gfx.buffer * SCREEN_HEIGHT;
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

typedef struct SND_Frame {
	int16_t left;
	int16_t right;
} SND_Frame;
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
void SND_audioCallback(void* userdata, uint8_t* stream, int len) { // plat_sound_callback
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
void SND_resizeBuffer(void) { // plat_sound_resize_buffer
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
int SND_resampleNone(SND_Frame frame) { // audio_resample_passthrough
	snd.buffer[snd.frame_in++] = frame;
	if (snd.frame_in >= snd.frame_count) snd.frame_in = 0;
	return 1;
}
int SND_resampleNear(SND_Frame frame) { // audio_resample_nearest
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
void SND_selectResampler(void) { // plat_sound_select_resampler
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
static struct PAD_Context {
	int is_pressed;
	int just_pressed;
	int just_released;
} pad;
void PAD_poll(void) {
	// reset transient state
	pad.just_pressed = 0;
	pad.just_released = 0;
	
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

// #define PAD_anyPressed() (pad.is_pressed!=BTN_NONE)
// #define PAD_justPressed(btn) (pad.just_pressed & (btn))
// #define PAD_isPressed(btn) (pad.is_pressed & (btn))
// #define PAD_justReleased(btn) (pad.just_released & (btn))

///////////////////////////////////////

static struct Game {
	char path[MAX_PATH];
	char name[MAX_PATH]; // TODO: rename to basename?
	void* data;
	size_t size;
} game;
static void Game_open(char* path) {
	strcpy((char*)game.path, path);
	strcpy((char*)game.name, strrchr(path, '/')+1);
		
	FILE *file = fopen(game.path, "r");
	if (file==NULL) {
		LOG_error("Error opening game: %s\n\t%s\n", game.path, strerror(errno));
		return;
	}
	
	fseek(file, 0, SEEK_END);
	game.size = ftell(file);
	
	rewind(file);
	game.data = malloc(game.size);
	fread(game.data, sizeof(uint8_t), game.size, file);
	
	fclose(file);
}
static void Game_close(void) {
	free(game.data);
}

///////////////////////////////

static struct Core {
	int initialized;
	
	const char tag[8]; // eg. GBC
	const char name[128]; // eg. gambatte
	const char version[128]; // eg. Gambatte (v0.5.0-netlink 7e02df6)
	
	double fps;
	double sample_rate;
	
	void* handle;
	void (*init)(void);
	void (*deinit)(void);
	
	void (*get_system_info)(struct retro_system_info *info);
	void (*get_system_av_info)(struct retro_system_av_info *info);
	void (*set_controller_port_device)(unsigned port, unsigned device);
	
	void (*reset)(void);
	void (*run)(void);
	size_t (*serialize_size)(void);
	bool (*serialize)(void *data, size_t size);
	bool (*unserialize)(const void *data, size_t size);
	bool (*load_game)(const struct retro_game_info *game);
	bool (*load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*unload_game)(void);
	unsigned (*get_region)(void);
	void *(*get_memory_data)(unsigned id);
	size_t (*get_memory_size)(unsigned id);
	retro_audio_buffer_status_callback_t audio_buffer_status;
} core;

///////////////////////////////////////
// saves and states

static void SRAM_getPath(char* filename) {
	sprintf(filename, SDCARD_PATH "/Saves/%s/%s.sav", core.tag, game.name);
}

static void SRAM_read(void) {
	size_t sram_size = core.get_memory_size(RETRO_MEMORY_SAVE_RAM);
	if (!sram_size) return;
	
	char filename[MAX_PATH];
	SRAM_getPath(filename);
	
	FILE *sram_file = fopen(filename, "r");
	if (!sram_file) return;

	void* sram = core.get_memory_data(RETRO_MEMORY_SAVE_RAM);

	if (!sram || !fread(sram, 1, sram_size, sram_file)) {
		LOG_error("Error reading SRAM data\n");
	}

	fclose(sram_file);
}
static void SRAM_write(void) {
	size_t sram_size = core.get_memory_size(RETRO_MEMORY_SAVE_RAM);
	if (!sram_size) return;
	
	char filename[MAX_PATH];
	SRAM_getPath(filename);
		
	FILE *sram_file = fopen(filename, "w");
	if (!sram_file) {
		LOG_error("Error opening SRAM file: %s\n", strerror(errno));
		return;
	}

	void *sram = core.get_memory_data(RETRO_MEMORY_SAVE_RAM);

	if (!sram || sram_size != fwrite(sram, 1, sram_size, sram_file)) {
		LOG_error("Error writing SRAM data to file\n");
	}

	fclose(sram_file);

	sync();
}

static int state_slot = 0;
static void State_getPath(char* filename) {
	sprintf(filename, SDCARD_PATH "/.userdata/" PLATFORM "/%s-%s/%s.st%i", core.tag, core.name, game.name, state_slot);
}

static void State_read(void) { // from picoarch
	size_t state_size = core.serialize_size();
	if (!state_size) return;

	void *state = calloc(1, state_size);
	if (!state) {
		LOG_error("Couldn't allocate memory for state\n");
		goto error;
	}

	char filename[MAX_PATH];
	State_getPath(filename);
	
	FILE *state_file = fopen(filename, "r");
	if (!state_file) {
		if (state_slot!=8) { // st8 is a default state in MiniUI and may not exist, that's okay
			LOG_error("Error opening state file: %s (%s)\n", filename, strerror(errno));
		}
		goto error;
	}

	if (state_size != fread(state, 1, state_size, state_file)) {
		LOG_error("Error reading state data from file: %s (%s)\n", filename, strerror(errno));
		goto error;
	}

	if (!core.unserialize(state, state_size)) {
		LOG_error("Error restoring save state: %s (%s)\n", filename, strerror(errno));
		goto error;
	}

error:
	if (state) free(state);
	if (state_file) fclose(state_file);
}
static void State_write(void) { // from picoarch
	size_t state_size = core.serialize_size();
	if (!state_size) return;

	void *state = calloc(1, state_size);
	if (!state) {
		LOG_error("Couldn't allocate memory for state\n");
		goto error;
	}

	char filename[MAX_PATH];
	State_getPath(filename);
	
	FILE *state_file = fopen(filename, "w");
	if (!state_file) {
		LOG_error("Error opening state file: %s (%s)\n", filename, strerror(errno));
		goto error;
	}

	if (!core.serialize(state, state_size)) {
		LOG_error("Error creating save state: %s (%s)\n", filename, strerror(errno));
		goto error;
	}

	if (state_size != fwrite(state, 1, state_size, state_file)) {
		LOG_error("Error writing state data to file: %s (%s)\n", filename, strerror(errno));
		goto error;
	}

error:
	if (state) free(state);
	if (state_file) fclose(state_file);

	sync();
}

///////////////////////////////

// callbacks
static struct retro_disk_control_ext_callback disk_control_ext;

static char sys_dir[MAX_PATH]; // TODO: move this somewhere else, maybe core.userdata?

// TODO: tmp, naive options
static struct {
	char key[128];
	char value[128];
} tmp_options[128];
static bool environment_callback(unsigned cmd, void *data) { // copied from picoarch initially
	// printf("environment_callback: %i\n", cmd); fflush(stdout);
	
	switch(cmd) {
	case RETRO_ENVIRONMENT_GET_OVERSCAN: { /* 2 */
		bool *out = (bool *)data;
		if (out)
			*out = true;
		break;
	}
	case RETRO_ENVIRONMENT_GET_CAN_DUPE: { /* 3 */
		bool *out = (bool *)data;
		if (out)
			*out = true;
		break;
	}
	case RETRO_ENVIRONMENT_SET_MESSAGE: { /* 6 */
		const struct retro_message *message = (const struct retro_message*)data;
		if (message) LOG_info("%s\n", message->msg);
		break;
	}
	// TODO: RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL 8
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: { /* 9 */
		puts("RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY");
		
		const char **out = (const char **)data;
		if (out)
			// TODO: set this once somewhere else
			// TODO: core.tag isn't available at this point
			// TODO: it only becomes available after we open the game...
			sprintf(sys_dir, SDCARD_PATH "/.userdata/%s/%s-%s", PLATFORM, core.tag, core.name);
			*out = sys_dir;
		break;
	}
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: { /* 10 */
		const enum retro_pixel_format *format = (enum retro_pixel_format *)data;

		if (*format != RETRO_PIXEL_FORMAT_RGB565) { // TODO: pull from platform.h?
			/* 565 is only supported format */
			return false;
		}
		break;
	}
	case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: { /* 11 */
		const struct retro_input_descriptor *vars = (const struct retro_input_descriptor *)data;
		if (vars) {
			// TODO: create an array of char* description indexed by id
			for (int i=0; vars[i].description; i++) {
				// vars[i].id == RETRO_DEVICE_ID_JOYPAD_*, vars[i].description = name
				printf("%i %s\n", vars[i].id, vars[i].description);
			}
			return false;
		}
	} break;
	case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE: { /* 13 */
		const struct retro_disk_control_callback *var =
			(const struct retro_disk_control_callback *)data;

		if (var) {
			memset(&disk_control_ext, 0, sizeof(struct retro_disk_control_ext_callback));
			memcpy(&disk_control_ext, var, sizeof(struct retro_disk_control_callback));
		}
		break;
	}
	// TODO: this is called whether using variables or options
	case RETRO_ENVIRONMENT_GET_VARIABLE: { /* 15 */
		struct retro_variable *var = (struct retro_variable *)data;
		if (var && var->key) {
			printf("get key: %s\n", var->key);
			for (int i=0; i<128; i++) {
				if (!strcmp(tmp_options[i].key, var->key)) {
					var->value = tmp_options[i].value;
					break;
				}
			}
			// var->value = options_get_value(var->key);
		}
		break;
	}
	// TODO: I think this is where the core reports its variables (the precursor to options)
	// TODO: this is called if RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION sets out to 0
	case RETRO_ENVIRONMENT_SET_VARIABLES: { /* 16 */
		const struct retro_variable *vars = (const struct retro_variable *)data;
		// options_free();
		if (vars) {
			// options_init_variables(vars);
			// load_config();
			
			for (int i=0; vars[i].key; i++) {
				// value appears to be NAME; DEFAULT|VALUE|VALUE|ETC
				printf("set var key: %s to value: %s\n", vars[i].key, vars[i].value);
			}
		}
		break;
	}
	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: { /* 17 */
		bool *out = (bool *)data;
		if (out)
			*out = false; // options_changed();
		break;
	}
	// case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: { /* 23 */
	//         struct retro_rumble_interface *iface =
	//            (struct retro_rumble_interface*)data;
	//
	//         PA_INFO("Setup rumble interface.\n");
	//         iface->set_rumble_state = pa_set_rumble_state;
	// 	break;
	// }
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: { /* 27 */
		struct retro_log_callback *log_cb = (struct retro_log_callback *)data;
		if (log_cb)
			log_cb->log = (void (*)(enum retro_log_level, const char*, ...))LOG_note; // same difference
		break;
	}
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: { /* 31 */
		const char **out = (const char **)data;
		if (out)
			*out = NULL; // save_dir;
		break;
	}
	// RETRO_ENVIRONMENT_GET_LANGUAGE 39
	case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: { /* 52 */
		bool *out = (bool *)data;
		if (out)
			*out = true;
		break;
	}
	case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: { /* 52 */
		unsigned *out = (unsigned *)data;
		if (out)
			*out = 1;
		break;
	}
	// TODO: options and variables are separate concepts use for the same thing...I think.
	// TODO: not used by gambatte
	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: { /* 53 */
		puts("RETRO_ENVIRONMENT_SET_CORE_OPTIONS");
		// options_free();
		if (data) {
			// options_init(*(const struct retro_core_option_definition **)data);
			// load_config();
			
			const struct retro_core_option_definition *vars = *(const struct retro_core_option_definition **)data;
			for (int i=0; vars[i].key; i++) {
				const struct retro_core_option_definition *var = &vars[i];
				// printf("set key: %s to value: %s (%s)\n", var->key, var->default_value, var->desc);
				printf("set option key: %s to value: %s\n", var->key, var->default_value);
			}
		}
		break;
	}
	
	// TODO: used by gambatte, fceumm (probably others)
	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: { /* 54 */
		puts("RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL");
		
		const struct retro_core_options_intl *options = (const struct retro_core_options_intl *)data;
		
		if (options && options->us) {
			// options_free();
			// options_init(options->us);
			// load_config();
			
			const struct retro_core_option_definition *vars = options->us;
			for (int i=0; vars[i].key; i++) {
				const struct retro_core_option_definition *var = &vars[i];
				// printf("set key: %s to value: %s (%s)\n", var->key, var->default_value, var->desc);
				printf("set core (intl) key: %s to value: %s\n", var->key, var->default_value);
				strcpy(tmp_options[i].key, var->key);
				strcpy(tmp_options[i].value, var->default_value);
			}
		}
		break;
	}
	// TODO: not used by gambatte
	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: { /* 55 */
		puts("RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY");
		
		const struct retro_core_option_display *display =
			(const struct retro_core_option_display *)data;

		if (display)
			printf("visible: %i (%s)\n", display->visible, display->key);
			// options_set_visible(display->key, display->visible);
		break;
	}
	case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION: { /* 57 */
		unsigned *out =	(unsigned *)data;
		if (out)
			*out = 1;
		break;
	}
	case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE: { /* 58 */
		const struct retro_disk_control_ext_callback *var =
			(const struct retro_disk_control_ext_callback *)data;

		if (var) {
			memcpy(&disk_control_ext, var, sizeof(struct retro_disk_control_ext_callback));
		}
		break;
	}
	// TODO: RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION 59
	case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK: { /* 62 */
		const struct retro_audio_buffer_status_callback *cb =
			(const struct retro_audio_buffer_status_callback *)data;
		if (cb) {
			core.audio_buffer_status = cb->callback;
		} else {
			core.audio_buffer_status = NULL;
		}
		break;
	}
	// TODO: not used by gambatte
	case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY: { /* 63 */
		puts("RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY");
		
		const unsigned *latency_ms = (const unsigned *)data;
		if (latency_ms) {
			unsigned frames = *latency_ms * core.fps / 1000;
			if (frames < 30)
				// audio_buffer_size_override = frames;
				printf("audio_buffer_size_override = %i\n", frames);
			// else
			// 	PA_WARN("Audio buffer change out of range (%d), ignored\n", frames);
		}
		break;
	}
	
	// TODO: RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE 64
	// TODO: RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK 69
	// TODO: UNKNOWN 70
	// TODO: UNKNOWN 65572
	// TODO: UNKNOWN 65578
	// TODO: UNKNOWN 65581
	// TODO: UNKNOWN 65587
	default:
		LOG_debug("Unsupported environment cmd: %u\n", cmd);
		return false;
	}

	return true;
}

///////////////////////////////

// from gambatte-dms
//from RGB565
#define cR(A) (((A) & 0xf800) >> 11)
#define cG(A) (((A) & 0x7e0) >> 5)
#define cB(A) ((A) & 0x1f)
//to RGB565
#define Weight2_3(A, B)  (((((cR(A) << 1) + (cR(B) * 3)) / 5) & 0x1f) << 11 | ((((cG(A) << 1) + (cG(B) * 3)) / 5) & 0x3f) << 5 | ((((cB(A) << 1) + (cB(B) * 3)) / 5) & 0x1f))
#define Weight3_2(A, B)  (((((cR(B) << 1) + (cR(A) * 3)) / 5) & 0x1f) << 11 | ((((cG(B) << 1) + (cG(A) * 3)) / 5) & 0x3f) << 5 | ((((cB(B) << 1) + (cB(A) * 3)) / 5) & 0x1f))

// TODO: flesh out
static void scale1x(int w, int h, int pitch, const void *src, void *dst) {
	// pitch of src image not src buffer! 
	// eg. gb has a 160 pixel wide image but 
	// gambatte uses a 256 pixel wide buffer
	// (only matters when using memcpy) 
	int src_pitch = w * SCREEN_BPP; 
	int src_stride = pitch / SCREEN_BPP;
	int dst_stride = SCREEN_PITCH / SCREEN_BPP;
	int cpy_pitch = MIN(src_pitch, SCREEN_PITCH);
	
	uint16_t* restrict src_row = (uint16_t*)src;
	uint16_t* restrict dst_row = (uint16_t*)dst;
	for (int y=0; y<h; y++) {
		memcpy(dst_row, src_row, cpy_pitch);
		dst_row += dst_stride;
		src_row += src_stride;
	}
	
}
static void scale2x(int w, int h, int pitch, const void *src, void *dst) {
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * SCREEN_PITCH * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row     ) = s;
			*(dst_row + 1 ) = s;
			
			// row 2
			*(dst_row + SCREEN_WIDTH    ) = s;
			*(dst_row + SCREEN_WIDTH + 1) = s;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}
static void scale3x(int w, int h, int pitch, const void *src, void *dst) {
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row    ) = s;
			*(dst_row + 1) = s;
			*(dst_row + 2) = s;
			
			// row 2
			*(dst_row + SCREEN_WIDTH    ) = s;
			*(dst_row + SCREEN_WIDTH + 1) = s;
			*(dst_row + SCREEN_WIDTH + 2) = s;

			// row 3
			*(dst_row + row3    ) = s;
			*(dst_row + row3 + 1) = s;
			*(dst_row + row3 + 2) = s;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale3x_lcd(int w, int h, int pitch, const void *src, void *dst) {
	uint16_t k = 0x0000;
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
            uint16_t r = (s & 0b1111100000000000);
            uint16_t g = (s & 0b0000011111100000);
            uint16_t b = (s & 0b0000000000011111);
			
			// row 1
			*(dst_row    ) = k;
			*(dst_row + 1) = g;
			*(dst_row + 2) = k;
			
			// row 2
			*(dst_row + SCREEN_WIDTH    ) = r;
			*(dst_row + SCREEN_WIDTH + 1) = g;
			*(dst_row + SCREEN_WIDTH + 2) = b;

			// row 3
			*(dst_row + row3    ) = r;
			*(dst_row + row3 + 1) = k;
			*(dst_row + row3 + 2) = b;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale3x_dmg(int w, int h, int pitch, const void *src, void *dst) {
	uint16_t g = 0xffff;
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t a = *src_row;
            uint16_t b = Weight3_2( a, g);
            uint16_t c = Weight2_3( a, g);
			
			// row 1
			*(dst_row    ) = b;
			*(dst_row + 1) = a;
			*(dst_row + 2) = a;
			
			// row 2
			*(dst_row + SCREEN_WIDTH    ) = b;
			*(dst_row + SCREEN_WIDTH + 1) = a;
			*(dst_row + SCREEN_WIDTH + 2) = a;

			// row 3
			*(dst_row + row3    ) = c;
			*(dst_row + row3 + 1) = b;
			*(dst_row + row3 + 2) = b;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale4x(int w, int h, int pitch, const void *src, void *dst) {
	int row3 = SCREEN_WIDTH * 2;
	int row4 = SCREEN_WIDTH * 3;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * SCREEN_PITCH * 4;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row    ) = s;
			*(dst_row + 1) = s;
			*(dst_row + 2) = s;
			*(dst_row + 3) = s;
			
			// row 2
			*(dst_row + SCREEN_WIDTH    ) = s;
			*(dst_row + SCREEN_WIDTH + 1) = s;
			*(dst_row + SCREEN_WIDTH + 2) = s;
			*(dst_row + SCREEN_WIDTH + 3) = s;

			// row 3
			*(dst_row + row3    ) = s;
			*(dst_row + row3 + 1) = s;
			*(dst_row + row3 + 2) = s;
			*(dst_row + row3 + 3) = s;

			// row 4
			*(dst_row + row4    ) = s;
			*(dst_row + row4 + 1) = s;
			*(dst_row + row4 + 2) = s;
			*(dst_row + row4 + 3) = s;

			src_row += 1;
			dst_row += 4;
		}
	}
}
static void scale(const void* src, int width, int height, int pitch, void* dst) {
	int scale_x = SCREEN_WIDTH / width;
	int scale_y = SCREEN_HEIGHT / height;
	int scale = MIN(scale_x,scale_y);
	int scale_w = width * scale;
	int scale_h = height * scale;
	int ox = (SCREEN_WIDTH - scale_w) / 2;
	int oy = (SCREEN_HEIGHT - scale_h) / 2;
		
	dst += (oy * SCREEN_PITCH) + (ox * SCREEN_BPP);
	
	switch (scale) {
		case 4: scale4x_n16((void*)src,dst,width,height,pitch,SCREEN_PITCH); break;
		case 3: scale3x_n16((void*)src,dst,width,height,pitch,SCREEN_PITCH); break;
		case 2: scale2x_n16((void*)src,dst,width,height,pitch,SCREEN_PITCH); break;
		default: scale1x_n16((void*)src,dst,width,height,pitch,SCREEN_PITCH); break;
		
		// case 4: scale4x(width,height,pitch,src,dst); break;
		// case 3: scale3x(width,height,pitch,src,dst); break;
		// case 3: scale3x_lcd(width,height,pitch,src,dst); break;
		// case 3: scale3x_dmg(width,height,pitch,src,dst); break;
		// case 2: scale2x(width,height,pitch,src,dst); break;
		// default: scale1x(width,height,pitch,src,dst); break;
	}
}

static void video_refresh_callback(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (!data) return;
	static int last_width = 0;
	static int last_height = 0;
	if (width!=last_width || height!=last_height) {
		last_width = width;
		last_height = height;
		GFX_clearAll();
	}
	scale(data,width,height,pitch,gfx.screen->pixels);
	GFX_flip(gfx.screen);
}

static void audio_sample_callback(int16_t left, int16_t right) {
	SND_batchSamples(&(const SND_Frame){left,right}, 1);
}
static size_t audio_sample_batch_callback(const int16_t *data, size_t frames) { 
	return SND_batchSamples((const SND_Frame*)data, frames);
};

static uint32_t buttons = 0;
static void input_poll_callback(void) {
	PAD_poll();
	
	// TODO: support remapping
	
	buttons = 0;
	if (PAD_isPressed(BTN_UP)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_UP;
	if (PAD_isPressed(BTN_DOWN)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN;
	if (PAD_isPressed(BTN_LEFT)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT;
	if (PAD_isPressed(BTN_RIGHT)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT;
	if (PAD_isPressed(BTN_A)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_A;
	if (PAD_isPressed(BTN_B)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_B;
	if (PAD_isPressed(BTN_X)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_X;
	if (PAD_isPressed(BTN_Y)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_Y;
	if (PAD_isPressed(BTN_START)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
	if (PAD_isPressed(BTN_SELECT)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_SELECT;
	if (PAD_isPressed(BTN_L1)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_L;
	if (PAD_isPressed(BTN_L2)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_L2;
	if (PAD_isPressed(BTN_R1)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_R;
	if (PAD_isPressed(BTN_R2)) buttons |= 1 << RETRO_DEVICE_ID_JOYPAD_R2;
}
static int16_t input_state_callback(unsigned port, unsigned device, unsigned index, unsigned id) { // copied from picoarch
	// id == RETRO_DEVICE_ID_JOYPAD_MASK or RETRO_DEVICE_ID_JOYPAD_*
	if (port == 0 && device == RETRO_DEVICE_JOYPAD && index == 0) {
		if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return buttons;
		return (buttons >> id) & 1;
	}
	return 0;
}

///////////////////////////////////////

void Core_getName(char* in_name, char* out_name) {
	strcpy(out_name, basename(in_name));
	char* tmp = strrchr(out_name, '_');
	tmp[0] = '\0';
}
void Core_open(const char* core_path, const char* tag_name) {
	LOG_info("inside Core_open\n");
	core.handle = dlopen(core_path, RTLD_LAZY);
	LOG_info("after dlopen\n");
	
	if (!core.handle) LOG_error("%s\n", dlerror());
	
	core.init = dlsym(core.handle, "retro_init");
	core.deinit = dlsym(core.handle, "retro_deinit");
	core.get_system_info = dlsym(core.handle, "retro_get_system_info");
	core.get_system_av_info = dlsym(core.handle, "retro_get_system_av_info");
	core.set_controller_port_device = dlsym(core.handle, "retro_set_controller_port_device");
	core.reset = dlsym(core.handle, "retro_reset");
	core.run = dlsym(core.handle, "retro_run");
	core.serialize_size = dlsym(core.handle, "retro_serialize_size");
	core.serialize = dlsym(core.handle, "retro_serialize");
	core.unserialize = dlsym(core.handle, "retro_unserialize");
	core.load_game = dlsym(core.handle, "retro_load_game");
	core.load_game_special = dlsym(core.handle, "retro_load_game_special");
	core.unload_game = dlsym(core.handle, "retro_unload_game");
	core.get_region = dlsym(core.handle, "retro_get_region");
	core.get_memory_data = dlsym(core.handle, "retro_get_memory_data");
	core.get_memory_size = dlsym(core.handle, "retro_get_memory_size");
	
	void (*set_environment_callback)(retro_environment_t);
	void (*set_video_refresh_callback)(retro_video_refresh_t);
	void (*set_audio_sample_callback)(retro_audio_sample_t);
	void (*set_audio_sample_batch_callback)(retro_audio_sample_batch_t);
	void (*set_input_poll_callback)(retro_input_poll_t);
	void (*set_input_state_callback)(retro_input_state_t);
	
	set_environment_callback = dlsym(core.handle, "retro_set_environment");
	set_video_refresh_callback = dlsym(core.handle, "retro_set_video_refresh");
	set_audio_sample_callback = dlsym(core.handle, "retro_set_audio_sample");
	set_audio_sample_batch_callback = dlsym(core.handle, "retro_set_audio_sample_batch");
	set_input_poll_callback = dlsym(core.handle, "retro_set_input_poll");
	set_input_state_callback = dlsym(core.handle, "retro_set_input_state");
	
	struct retro_system_info info = {};
	core.get_system_info(&info);
	
	Core_getName((char*)core_path, (char*)core.name);
	sprintf((char*)core.version, "%s (%s)", info.library_name, info.library_version);
	strcpy((char*)core.tag, tag_name);

	set_environment_callback(environment_callback);
	set_video_refresh_callback(video_refresh_callback);
	set_audio_sample_callback(audio_sample_callback);
	set_audio_sample_batch_callback(audio_sample_batch_callback);
	set_input_poll_callback(input_poll_callback);
	set_input_state_callback(input_state_callback);
}
void Core_init(void) {
	core.init();
	core.initialized = 1;
}
void Core_load(void) {
	LOG_info("inside Core_load\n");
	
	struct retro_game_info game_info;
	game_info.path = game.path;
	game_info.data = game.data;
	game_info.size = game.size;
	
	core.load_game(&game_info);
	LOG_info("after core.load_game\n");
	
	SRAM_read();
	LOG_info("after SRAM_read\n");
	
	// NOTE: must be called after core.load_game!
	struct retro_system_av_info av_info = {};
	core.get_system_av_info(&av_info);
	LOG_info("after core.get_system_av_info\n");
	
	double a = av_info.geometry.aspect_ratio;
	int w = av_info.geometry.base_width;
	int h = av_info.geometry.base_height;
	// char r[8];
	// getRatio(a, r);
	// LOG_info("after getRatio\n");
	
	core.fps = av_info.timing.fps;
	core.sample_rate = av_info.timing.sample_rate;

	printf("%s\n%s\n", core.tag, core.version);
	// printf("%dx%d (%s)\n", w,h,r);
	printf("%f\n%f\n", core.fps, core.sample_rate);
	fflush(stdout);
}
void Core_unload(void) {
	SND_quit();
}
void Core_quit(void) {
	if (core.initialized) {
		SRAM_write();
		core.unload_game();
		core.deinit();
		core.initialized = 0;
	}
}
void Core_close(void) {
	if (core.handle) dlclose(core.handle);
}

int main(int argc , char* argv[]) {
	// system("touch /tmp/wait");

	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/gambatte_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Game Boy Color (GBC)/Legend of Zelda, The - Link's Awakening DX (USA, Europe) (Rev 2) (SGB Enhanced) (GB Compatible).gbc";
	// char* rom_path = "/mnt/sdcard/Roms/Game Boy Color (GBC)/Dragon Warrior I & II (USA) (SGB Enhanced).gbc";
	// char* tag_name = "GBC";
	// char* rom_path = "/mnt/sdcard/Roms/Game Boy (GB)/Super Mario Land (World) (Rev A).gb";
	// char* rom_path = "/mnt/sdcard/Roms/Game Boy (GB)/Dr. Mario (World).gb";
	// char* tag_name = "GB";
	
	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/gpsp_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Game Boy Advance (GBA)/Metroid Zero Mission.gba";
	// char* tag_name = "GBA";
	
	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/fceumm_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Nintendo (FC)/Castlevania 3 - Dracula's Curse (U).nes";
	// char* rom_path = "/mnt/sdcard/Roms/Nintendo (FC)/Mega Man 2 (U).nes";
	// char* tag_name = "FC";
	
	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/picodrive_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Genesis (MD)/Sonic The Hedgehog (USA, Europe).md";
	// char* tag_name = "MD";
	
	char* core_path = "/mnt/sdcard/.system/rg35xx/cores/snes9x2005_plus_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Super Nintendo (SFC)/Super Mario World (USA).sfc";
	// char* rom_path = "/mnt/sdcard/Roms/Super Nintendo (SFC)/Super Mario World 2 - Yoshi's Island (USA, Asia) (Rev 1).sfc";
	char* rom_path = "/mnt/sdcard/Roms/Super Nintendo (SFC)/Final Fantasy III (USA) (Rev 1).sfc";
	char* tag_name = "SFC";

	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/pcsx_rearmed_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/PlayStation (PS)/Castlevania - Symphony of the Night (USA)/Castlevania - Symphony of the Night (USA).cue";
	// char* rom_path = "/mnt/sdcard/Roms/PlayStation (PS)/Final Fantasy VII (USA)/Final Fantasy VII (USA).m3u";
	// char* tag_name = "PS";
	
	// char* core_path = "/mnt/sdcard/.system/rg35xx/cores/pokemini_libretro.so";
	// char* rom_path = "/mnt/sdcard/Roms/Pokémon mini (PKM)/Pokemon Tetris (Europe) (En,Ja,Fr).min";
	// char* tag_name = "PKM";
	
	// char core_path[MAX_PATH]; strcpy(core_path, argv[1]);
	// char rom_path[MAX_PATH]; strcpy(rom_path, argv[2]);
	// char tag_name[MAX_PATH]; strcpy(tag_name, argv[3]);
	
	LOG_info("core_path: %s\n", core_path);
	LOG_info("rom_path: %s\n", rom_path);
	LOG_info("tag_name: %s\n", tag_name);
	
	SDL_Surface* screen = GFX_init();
	Core_open(core_path, tag_name); 		LOG_info("after Core_open\n");
	Core_init(); 							LOG_info("after Core_init\n");
	Game_open(rom_path); 					LOG_info("after Game_open\n");
	Core_load();  							LOG_info("after Core_load\n");
	SND_init(core.sample_rate, core.fps);	LOG_info("after SND_init\n");
	State_read();							LOG_info("after State_read\n");
	
	while (1) {		
		if (PAD_justPressed(BTN_POWER)) {
			system("rm /tmp/minui_exec");
			break;
		}
		
		// still not working
		// if (PAD_justPressed(BTN_L1)) State_read();
		// else if (PAD_justPressed(BTN_R1)) State_write();
		core.run();
	}
	
	Game_close();
	Core_unload();

	Core_quit();
	Core_close(); LOG_info("after Core_close\n");
	
	SDL_FreeSurface(screen);
	GFX_quit();
	
	return EXIT_SUCCESS;
}
