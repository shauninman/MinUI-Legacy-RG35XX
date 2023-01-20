#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <msettings.h>

#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "libretro.h"
#include "defines.h"
#include "utils.h"
#include "api.h"
#include "scaler_neon.h"

static SDL_Surface* screen;
static int quit;
static int show_menu;

///////////////////////////////////////

static struct Game {
	char path[MAX_PATH];
	char name[MAX_PATH]; // TODO: rename to basename?
	char m3u_path[MAX_PATH];
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
	
	// m3u-based?
	char* tmp;
	char m3u_path[256];
	char base_path[256];
	char dir_name[256];

	strcpy(m3u_path, game.path);
	tmp = strrchr(m3u_path, '/') + 1;
	tmp[0] = '\0';
	
	strcpy(base_path, m3u_path);
	
	tmp = strrchr(m3u_path, '/');
	tmp[0] = '\0';

	tmp = strrchr(m3u_path, '/');
	strcpy(dir_name, tmp);
	
	tmp = m3u_path + strlen(m3u_path); 
	strcpy(tmp, dir_name);
	
	tmp = m3u_path + strlen(m3u_path);
	strcpy(tmp, ".m3u");
	
	if (exists(m3u_path)) {
		strcpy(game.m3u_path, m3u_path);
		strcpy((char*)game.name, strrchr(m3u_path, '/')+1);
	}
	else {
		game.m3u_path[0] = '\0';
	}
}
static void Game_close(void) {
	free(game.data);
	POW_setRumble(0); // just in case
}

static struct retro_disk_control_ext_callback disk_control_ext;
static void Game_changeDisc(char* path) {
	
	if (exactMatch(game.path, path) || !exists(path)) return;
	
	Game_close();
	Game_open(path);
	
	struct retro_game_info game_info = {};
	game_info.path = game.path;
	game_info.data = game.data;
	game_info.size = game.size;
	
	disk_control_ext.replace_image_index(0, &game_info);
	putFile(CHANGE_DISC_PATH, path); // MinUI still needs to know this to update recents.txt
}

///////////////////////////////

static struct Core {
	int initialized;
	
	const char tag[8]; // eg. GBC
	const char name[128]; // eg. gambatte
	const char version[128]; // eg. Gambatte (v0.5.0-netlink 7e02df6)
	
	const char config_dir[MAX_PATH]; // eg. /mnt/sdcard/.userdata/rg35xx/GB-gambatte
	const char saves_dir[MAX_PATH]; // eg. /mnt/sdcard/Saves/GB
	const char bios_dir[MAX_PATH]; // eg. /mnt/sdcard/Bios/GB
	
	double fps;
	double sample_rate;
	double aspect_ratio;
	
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
	sprintf(filename, "%s/%s.sav", core.saves_dir, game.name);
}

static void SRAM_read(void) {
	size_t sram_size = core.get_memory_size(RETRO_MEMORY_SAVE_RAM);
	if (!sram_size) return;
	
	char filename[MAX_PATH];
	SRAM_getPath(filename);
	printf("sav path (read): %s\n", filename);
	
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
	printf("sav path (write): %s\n", filename);
		
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
	sprintf(filename, "%s/%s.st%i", core.config_dir, game.name, state_slot);
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
static void State_autosave(void) {
	int last_state_slot = state_slot;
	state_slot = AUTO_RESUME_SLOT;
	State_write();
	state_slot = last_state_slot;
}
static void State_resume(void) {
	if (!exists(RESUME_SLOT_PATH)) return;
	
	int last_state_slot = state_slot;
	state_slot = getInt(RESUME_SLOT_PATH);
	unlink(RESUME_SLOT_PATH);
	State_read();
	state_slot = last_state_slot;
}

///////////////////////////////

// callbacks

// TODO: tmp, naive options
static int tmp_options_changed = 0;
static struct {
	char key[128];
	char value[128];
} tmp_options[128];
static bool set_rumble_state(unsigned port, enum retro_rumble_effect effect, uint16_t strength) {
	// TODO: handle other args? not sure I can
	POW_setRumble(strength);
}
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
		const char **out = (const char **)data;
		if (out) {
			*out = core.bios_dir;
		}
		
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
		puts("RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS");
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
		puts("RETRO_ENVIRONMENT_GET_VARIABLE");
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
		puts("RETRO_ENVIRONMENT_SET_VARIABLES");
		const struct retro_variable *vars = (const struct retro_variable *)data;
		// options_free();
		if (vars) {
			// options_init_variables(vars);
			// load_config();
			
			for (int i=0; vars[i].key; i++) {
				// value appears to be NAME; DEFAULT|VALUE|VALUE|ETC
				printf("set bulk var key: %s to value: %s\n", vars[i].key, vars[i].value);
			}
		}
		break;
	}
	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: { /* 17 */
		bool *out = (bool *)data;
		if (out) {
			*out = tmp_options_changed; // options_changed();
			tmp_options_changed = 0;
		}
		break;
	}
	case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: { /* 23 */
	        struct retro_rumble_interface *iface = (struct retro_rumble_interface*)data;

	        LOG_info("Setup rumble interface.\n");
	        iface->set_rumble_state = set_rumble_state;
		break;
	}
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: { /* 27 */
		struct retro_log_callback *log_cb = (struct retro_log_callback *)data;
		if (log_cb)
			log_cb->log = (void (*)(enum retro_log_level, const char*, ...))LOG_note; // same difference
		break;
	}
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: { /* 31 */
		const char **out = (const char **)data;
		if (out)
			*out = core.saves_dir; // save_dir;
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

				// TODO: tmp, patch or override cores
				char *default_value = (char*)var->default_value;
				if (!strcmp("gpsp_save_method", var->key)) {
					default_value = "libretro";
				}
				else if (!strcmp("gambatte_gb_colorization", var->key)) {
					default_value = "internal";
				}
				else if (!strcmp("gambatte_gb_internal_palette", var->key)) {
					default_value = "TWB64 - Pack 1";
				}
				else if (!strcmp("gambatte_gb_palette_twb64_1", var->key)) {
					default_value = "TWB64 038 - Pokemon mini Ver.";
				}
				
				printf("set core (intl) key: %s to value: %s\n", var->key, default_value);
				strcpy(tmp_options[i].key, var->key);
				strcpy(tmp_options[i].value, default_value);
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
	// TODO: used by gambatte for L/R palette switching (seems like it needs to return true even if data is NULL to indicate support)
	// TODO: these should be overridden to be disabled by default because ick
	case RETRO_ENVIRONMENT_SET_VARIABLE: {
		puts("RETRO_ENVIRONMENT_SET_VARIABLE");
		
		const struct retro_variable *var = (const struct retro_variable *)data;
		if (var && var->key) {
			printf("\tset individual var key: %s to value: %s\n", var->key, var->value);
			for (int i=0; i<128; i++) {
				if (!strcmp(tmp_options[i].key, var->key)) {
					strcpy(tmp_options[i].value, var->value);
					tmp_options_changed = 1;
					break;
				}
			}
			break;
		}

		int *out = (int *)data;
		if (out) *out = 1;
		
		break;
	}
	// TODO: these unknowns are probably some flag OR'd to RETRO_ENVIRONMENT_EXPERIMENTAL
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

static int cpu_ticks = 0;
static int fps_ticks = 0;
static uint32_t sec_start = 0;

// TODO: flesh out
static scale_neon_t scaler;
static int dst_offset,dst_w,dst_h;
static void scaleNull(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {}
static void scale1x(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	// pitch of src image not src buffer!
	// eg. gb has a 160 pixel wide image but 
	// gambatte uses a 256 pixel wide buffer
	// (only matters when using memcpy) 
	int src_pitch = w * SCREEN_BPP; 
	int src_stride = pitch / SCREEN_BPP;
	int dst_stride = dst_pitch / SCREEN_BPP;
	int cpy_pitch = MIN(src_pitch, dst_pitch);
	
	uint16_t* restrict src_row = (uint16_t*)src;
	uint16_t* restrict dst_row = (uint16_t*)dst;
	for (int y=0; y<h; y++) {
		memcpy(dst_row, src_row, cpy_pitch);
		dst_row += dst_stride;
		src_row += src_stride;
	}
	
}
static void scale2x(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 2;
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
static void scale2x_lcd(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
            uint16_t r = (s & 0b1111100000000000);
            uint16_t g = (s & 0b0000011111100000);
            uint16_t b = (s & 0b0000000000011111);
			
			*(dst_row                       ) = r;
			*(dst_row + 1                   ) = b;
			
			*(dst_row + SCREEN_WIDTH * 1    ) = g;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = k;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}
static void scale2x_scanline(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			*(dst_row     ) = s;
			*(dst_row + 1 ) = s;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}
static void scale3x(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 3;
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
static void scale3x_lcd(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	uint16_t k = 0x0000;
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 3;
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
static void scale3x_dmg(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	uint16_t g = 0xffff;
	int row3 = SCREEN_WIDTH * 2;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 3;
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
static void scale3x_scanline(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	// uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row                       ) = s;
			*(dst_row                    + 1) = s;
			// *(dst_row                    + 2) = k;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = s;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = s;
			// *(dst_row + SCREEN_WIDTH * 1 + 2) = k;

			// row 3
			// *(dst_row + SCREEN_WIDTH * 2    ) = k;
			// *(dst_row + SCREEN_WIDTH * 2 + 1) = k;
			// *(dst_row + SCREEN_WIDTH * 2 + 2) = k;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale4x(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	int row3 = SCREEN_WIDTH * 2;
	int row4 = SCREEN_WIDTH * 3;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* restrict src_row = (void*)src + y * pitch;
		uint16_t* restrict dst_row = (void*)dst + y * dst_pitch * 4;
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
static void scaleNN(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	int dy = -dst_h;
	unsigned lines = h;
	bool copy = false;
	size_t cpy_w = dst_w * SCREEN_BPP;

	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;

		if (copy) {
			copy = false;
			memcpy(dst, dst - dst_pitch, cpy_w);
			dst += dst_pitch;
			dy += h;
		} else if (dy < 0) {
			int col = w;
			while(col--) {
				while (dx < 0) {
					*pdst16++ = *psrc16;
					dx += w;
				}

				dx -= dst_w;
				psrc16++;
			}

			dst += dst_pitch;
			dy += h;
		}

		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		} else {
			copy = true;
		}
	}
}
static void scaleNN_scanline(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	int dy = -dst_h;
	unsigned lines = h;
	int row = 0;
	
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;
		
		if (row%2==0) {
			int col = w;
			while(col--) {
				while (dx < 0) {
					*pdst16++ = *psrc16;
					dx += w;
				}

				dx -= dst_w;
				psrc16++;
			}
		}

		dst += dst_pitch;
		dy += h;
				
		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		}
		row += 1;
	}
}
static void scaleNN_text(void* __restrict src, void* __restrict dst, uint32_t w, uint32_t h, uint32_t pitch, uint32_t dst_pitch) {
	int dy = -dst_h;
	unsigned lines = h;
	bool copy = false;

	size_t cpy_w = dst_w * SCREEN_BPP;
	
	int safe = w - 1; // don't look behind when there's nothing to see
	uint16_t l1,l2;
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;
		l1 = l2 = 0x0;
		
		if (copy) {
			copy = false;
			memcpy(dst, dst - SCREEN_PITCH, cpy_w);
			dst += SCREEN_PITCH;
			dy += h;
		} else if (dy < 0) {
			int col = w;
			while(col--) {
				int d = 0;
				if (col<safe && l1!=l2) {
					// https://stackoverflow.com/a/71086522/145965
					uint16_t r = (l1 >> 10) & 0x3E;
					uint16_t g = (l1 >> 5) & 0x3F;
					uint16_t b = (l1 << 1) & 0x3E;
					uint16_t luma = (r * 218) + (g * 732) + (b * 74);
					luma = (luma >> 10) + ((luma >> 9) & 1); // 0-63
					d = luma > 24;
				}

				uint16_t s = *psrc16;
				
				while (dx < 0) {
					*pdst16++ = d ? l1 : s;
					dx += w;

					l2 = l1;
					l1 = s;
					d = 0;
				}

				dx -= dst_w;
				psrc16++;
			}

			dst += SCREEN_PITCH;
			dy += h;
		}

		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		} else {
			copy = true;
		}
	}
}
static void selectScaler(int width, int height, int pitch) {
	scaler = scaleNull;
	int use_nearest = 0;
	
	int scale_x = SCREEN_WIDTH / width;
	int scale_y = SCREEN_HEIGHT / height;
	int scale = MIN(scale_x,scale_y);
	double near_ratio;
	
	if (scale<=1) {
		use_nearest = 1;
		if (scale_y>scale_x) { printf("NN:A %ix%i (%s)\n", width,height,game.name); fflush(stdout);
			dst_h = height * scale_y;
			
			// if the aspect ratio of an unmodified
			// w to dst_h is within 20% of the target
			// aspect_ratio don't force
			near_ratio = (double)width / dst_h / core.aspect_ratio;
			if (near_ratio>=0.79 && near_ratio<=1.21) {
				dst_w = width;
			}
			else {
				dst_w = dst_h * core.aspect_ratio;
				dst_w -= dst_w % 2;
			}
			
			if (dst_w>SCREEN_WIDTH) {
				dst_w = SCREEN_WIDTH;
				dst_h = dst_w / core.aspect_ratio;
				dst_h -= dst_w % 2;
				if (dst_h>SCREEN_HEIGHT) dst_h = SCREEN_HEIGHT;
			}
		}
		else if (scale_x>scale_y) {  printf("NN:B %ix%i (%s)\n", width,height,game.name); fflush(stdout);
			dst_w = width * scale_x;
			
			// see above
			near_ratio = (double)dst_w / height / core.aspect_ratio;
			if (near_ratio>=0.79 && near_ratio<=1.21) {
				dst_h = height;
			}
			else {
				dst_h = dst_w / core.aspect_ratio;
				dst_h -= dst_w % 2;
			}
		
			if (dst_h>SCREEN_HEIGHT) {
				dst_h = SCREEN_HEIGHT;
				dst_w = dst_h * core.aspect_ratio;
				dst_w -= dst_w % 2;
				if (dst_w>SCREEN_WIDTH) dst_w = SCREEN_WIDTH;
			}
		}
		else {  printf("NN:C %ix%i (%s)\n", width,height,game.name); fflush(stdout);
			dst_w = width * scale_x;
			dst_h = height * scale_y;
		
			// see above
			near_ratio = (double)dst_w / dst_h / core.aspect_ratio;
			if (near_ratio>=0.79 && near_ratio<=1.21) {
				// close enough
			}
			else {
				if (dst_h>dst_w) {
					dst_w = dst_h * core.aspect_ratio;
					dst_w -= dst_w % 2;
				}
				else {
					dst_h = dst_w / core.aspect_ratio;
					dst_h -= dst_w % 2;
				}
			}
		
			if (dst_w>SCREEN_WIDTH) {
				dst_w = SCREEN_WIDTH;
			}
			if (dst_h>SCREEN_HEIGHT) {
				dst_h = SCREEN_HEIGHT;
			}
		}
	}
	else {
		dst_w = width * scale;
		dst_h = height * scale;
	}
	
	int ox = (SCREEN_WIDTH - dst_w) / 2;
	int oy = (SCREEN_HEIGHT - dst_h) / 2;
	dst_offset = (oy * SCREEN_PITCH) + (ox * SCREEN_BPP);

	if (use_nearest) 
		scaler = scaleNN_text;
	else {
		switch (scale) {
			// eggs-optimized scalers
			case 4: 	scaler = scale4x_n16; break;
			case 3: 	scaler = scale3x_n16; break;
			case 2: 	scaler = scale2x_n16; break;
			default:	scaler = scale1x; break;
			
			// my lesser scalers :sweat_smile:
			// case 4: 	scaler = scale4x; break;
			// case 3: 	scaler = scale3x; break;
			// case 3: 	scaler = scale3x_dmg; break;
			// case 3: 	scaler = scale3x_lcd; break;
			// case 3: 	scaler = scale3x_scanline; break;
			// case 2: 	scaler = scale2x; break;
			// case 2: 	scaler = scale2x_lcd; break;
			// case 2: 	scaler = scale2x_scanline; break;
			// default:	scaler = scale1x; break;
		}
	}
}

static void video_refresh_callback(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (!data) return;
	fps_ticks += 1;
	
	static int last_width = 0;
	static int last_height = 0;
	if (width!=last_width || height!=last_height) {
		last_width = width;
		last_height = height;
		selectScaler(width,height,pitch);
		GFX_clearAll();
	}
	scaler((void*)data,screen->pixels+dst_offset,width,height,pitch,SCREEN_PITCH);
	
	if (0) {
		static int frame = 0;
		int w = 8;
		int h = 16;
		int fps = 60;
		int x = frame * w;
		
		void *dst = screen->pixels;
	
		dst += (SCREEN_WIDTH - (w * fps)) / 2 * SCREEN_BPP;

		void* _dst = dst;
		memset(_dst, 0, (h * SCREEN_PITCH));
		for (int y=0; y<h; y++) {
			memset(_dst-SCREEN_BPP, 0xff, SCREEN_BPP);
			memset(_dst+(w * fps * SCREEN_BPP), 0xff, SCREEN_BPP);
			_dst += SCREEN_PITCH;
		}

		dst += (x * SCREEN_BPP);

		for (int y=0; y<h; y++) {
			memset(dst, 0xff, w * SCREEN_BPP);
			dst += SCREEN_PITCH;
		}

		frame += 1;
		if (frame>=fps) frame -= fps;
	}
	
	GFX_flip(screen);
	
	return; // TODO: tmp
	
	// measure framerate
	static int ticks = 0;
	static uint32_t start = -1;
	ticks += 1;
	uint32_t now = SDL_GetTicks();
	if (start==-1) start = now;
	if (now-start>=1000) {
		start = now;
		printf("fps: %i\n", ticks);
		fflush(stdout);
		ticks = 0;
	}
}

static void audio_sample_callback(int16_t left, int16_t right) {
	SND_batchSamples(&(const SND_Frame){left,right}, 1);
}
static size_t audio_sample_batch_callback(const int16_t *data, size_t frames) { 
	return SND_batchSamples((const SND_Frame*)data, frames);
};

static void Menu_beforeSleep(void);
static void Menu_afterSleep(void);

static uint32_t buttons = 0; // RETRO_DEVICE_ID_JOYPAD_* buttons
static int ignore_menu = 0;
static void input_poll_callback(void) {
	PAD_poll();




	// TODO: too heavy? maybe but regardless,
	// this will cause it to go to sleep after 
	// 30 seconds--even while playing!
	POW_update(NULL,NULL, Menu_beforeSleep, Menu_afterSleep);

	if (PAD_justPressed(BTN_MENU)) {
		ignore_menu = 0;
	}
	if (PAD_isPressed(BTN_MENU) && (PAD_isPressed(BTN_VOL_UP) || PAD_isPressed(BTN_VOL_DN))) {
		ignore_menu = 1;
	}
	
	if (!ignore_menu && PAD_justReleased(BTN_MENU)) {
		show_menu = 1;
	}
	
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
	
	sprintf((char*)core.config_dir, SDCARD_PATH "/.userdata/" PLATFORM "/%s-%s", core.tag, core.name);
	sprintf((char*)core.saves_dir, SDCARD_PATH "/Saves/%s", core.tag);
	sprintf((char*)core.bios_dir, SDCARD_PATH "/Bios/%s", core.tag);
	char cmd[512];
	sprintf(cmd, "mkdir -p \"%s\"", core.config_dir);
	system(cmd);

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
	// LOG_info("inside Core_load\n");
	
	struct retro_game_info game_info;
	game_info.path = game.path;
	game_info.data = game.data;
	game_info.size = game.size;
	
	core.load_game(&game_info);
	// LOG_info("after core.load_game\n");
	
	SRAM_read();
	// LOG_info("after SRAM_read\n");
	
	// NOTE: must be called after core.load_game!
	struct retro_system_av_info av_info = {};
	core.get_system_av_info(&av_info);
	// LOG_info("after core.get_system_av_info\n");
	
	// double a = av_info.geometry.aspect_ratio;
	// int w = av_info.geometry.base_width;
	// int h = av_info.geometry.base_height;
	// char r[8];
	// getRatio(a, r);
	// LOG_info("after getRatio\n");
	
	core.fps = av_info.timing.fps;
	core.sample_rate = av_info.timing.sample_rate;
	double a = av_info.geometry.aspect_ratio;
	if (a<=0) a = (double)av_info.geometry.base_width / av_info.geometry.base_height;
	core.aspect_ratio = a;
	
	printf("aspect_ratio: %f\n", a);

	// printf("%s\n%s\n", core.tag, core.version);
	// printf("%dx%d (%s)\n", w,h,r);
	// printf("%f\n%f\n", core.fps, core.sample_rate);
	fflush(stdout);
}
void Core_reset(void) {
	core.reset();
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

///////////////////////////////////////

#define MENU_ITEM_COUNT 5
#define MENU_SLOT_COUNT 8

enum {
	ITEM_CONT,
	ITEM_SAVE,
	ITEM_LOAD,
	ITEM_OPTS,
	ITEM_QUIT,
};

enum {
	STATUS_CONT =  0,
	STATUS_SAVE =  1,
	STATUS_LOAD = 11,
	STATUS_OPTS = 23,
	STATUS_DISC = 24,
	STATUS_QUIT = 30
};

static struct Menu {
	int initialized;
	SDL_Surface* overlay;
	char* items[MENU_ITEM_COUNT];
	int slot;
} menu = {
	.items = {
		[ITEM_CONT] = "Continue",
		[ITEM_SAVE] = "Save",
		[ITEM_LOAD] = "Load",
		[ITEM_OPTS] = "Reset",
		[ITEM_QUIT] = "Quit",
	}
};

typedef struct __attribute__((__packed__)) uint24_t {
	uint8_t a,b,c;
} uint24_t;
static SDL_Surface* Menu_thumbnail(SDL_Surface* src_img) {
	SDL_Surface* dst_img = SDL_CreateRGBSurface(0,SCREEN_WIDTH/2, SCREEN_HEIGHT/2,src_img->format->BitsPerPixel,src_img->format->Rmask,src_img->format->Gmask,src_img->format->Bmask,src_img->format->Amask);

	uint8_t* src_px = src_img->pixels;
	uint8_t* dst_px = dst_img->pixels;
	int step = dst_img->format->BytesPerPixel;
	int step2 = step * 2;
	int stride = src_img->pitch;
	for (int y=0; y<dst_img->h; y++) {
		for (int x=0; x<dst_img->w; x++) {
			switch(step) {
				case 1:
					*dst_px = *src_px;
					break;
				case 2:
					*(uint16_t*)dst_px = *(uint16_t*)src_px;
					break;
				case 3:
					*(uint24_t*)dst_px = *(uint24_t*)src_px;
					break;
				case 4:
					*(uint32_t*)dst_px = *(uint32_t*)src_px;
					break;
			}
			dst_px += step;
			src_px += step2;
		}
		src_px += stride;
	}

	return dst_img;
}

void Menu_init(void) {
	menu.overlay = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, 0, 0, 0, 0);
	SDL_SetAlpha(menu.overlay, SDL_SRCALPHA, 0x80);
	SDL_FillRect(menu.overlay, NULL, 0);
}
void Menu_quit(void) {
	SDL_FreeSurface(menu.overlay);
}
void Menu_beforeSleep(void) {
	State_autosave();
	putFile(AUTO_RESUME_PATH, game.path + strlen(SDCARD_PATH));
}
void Menu_afterSleep(void) {
	unlink(AUTO_RESUME_PATH);
}
void Menu_loop(void) {
	POW_enableAutosleep();
	PAD_reset();
	
	// current screen is on the previous buffer
	SDL_Surface* backing = GFX_getBufferCopy();
	
	// path and string things
	char* tmp;
	char rom_name[256]; // without extension or cruft
	char slot_path[256];
	char emu_name[256];
	char minui_dir[256];
		
	getEmuName(game.path, emu_name);
	sprintf(minui_dir, USERDATA_PATH "/.minui/%s", emu_name);
	mkdir(minui_dir, 0755);
	
	int rom_disc = -1;
	int disc = rom_disc;
	int total_discs = 0;
	char disc_name[16];
	char* disc_paths[9]; // up to 9 paths, Arc the Lad Collection is 7 discs
	char base_path[256]; // used below too when status==kItemSave
	
	if (game.m3u_path[0]) {
		strcpy(base_path, game.m3u_path);
		tmp = strrchr(base_path, '/') + 1;
		tmp[0] = '\0';
		
		//read m3u file
		FILE* file = fopen(game.m3u_path, "r");
		if (file) {
			char line[256];
			while (fgets(line,256,file)!=NULL) {
				normalizeNewline(line);
				trimTrailingNewlines(line);
				if (strlen(line)==0) continue; // skip empty lines
		
				char disc_path[256];
				strcpy(disc_path, base_path);
				tmp = disc_path + strlen(disc_path);
				strcpy(tmp, line);
				
				// found a valid disc path
				if (exists(disc_path)) {
					disc_paths[total_discs] = strdup(disc_path);
					// matched our current disc
					if (exactMatch(disc_path, game.path)) {
						rom_disc = total_discs;
						disc = rom_disc;
						sprintf(disc_name, "Disc %i", disc+1);
					}
					total_discs += 1;
				}
			}
			fclose(file);
		}
	}
	
	// shares saves across multi-disc games too
	sprintf(slot_path, "%s/%s.txt", minui_dir, game.name);
	getDisplayName(game.name, rom_name);
	
	int selected = 0; // resets every launch
	if (exists(slot_path)) menu.slot = getInt(slot_path);
	if (menu.slot==8) menu.slot = 0;
	
	char save_path[256];
	char bmp_path[256];
	char txt_path[256];
	int save_exists = 0;
	int preview_exists = 0;
	
	int status = STATUS_CONT; // TODO: tmp?
	int show_setting = 0;
	int dirty = 1;
	while (show_menu) {
		GFX_startFrame();
		uint32_t frame_start = SDL_GetTicks();

		PAD_poll();
		
		if (PAD_justPressed(BTN_UP)) {
			selected -= 1;
			if (selected<0) selected += MENU_ITEM_COUNT;
			dirty = 1;
		}
		else if (PAD_justPressed(BTN_DOWN)) {
			selected += 1;
			if (selected>=MENU_ITEM_COUNT) selected -= MENU_ITEM_COUNT;
			dirty = 1;
		}
		else if (PAD_justPressed(BTN_LEFT)) {
			if (total_discs>1 && selected==ITEM_CONT) {
				disc -= 1;
				if (disc<0) disc += total_discs;
				dirty = 1;
				sprintf(disc_name, "Disc %i", disc+1);
			}
			else if (selected==ITEM_SAVE || selected==ITEM_LOAD) {
				menu.slot -= 1;
				if (menu.slot<0) menu.slot += MENU_SLOT_COUNT;
				dirty = 1;
			}
		}
		else if (PAD_justPressed(BTN_RIGHT)) {
			if (total_discs>1 && selected==ITEM_CONT) {
				disc += 1;
				if (disc==total_discs) disc -= total_discs;
				dirty = 1;
				sprintf(disc_name, "Disc %i", disc+1);
			}
			else if (selected==ITEM_SAVE || selected==ITEM_LOAD) {
				menu.slot += 1;
				if (menu.slot>=MENU_SLOT_COUNT) menu.slot -= MENU_SLOT_COUNT;
				dirty = 1;
			}
		}
		
		if (dirty && (selected==ITEM_SAVE || selected==ITEM_LOAD)) {
			int last_slot = state_slot;
			state_slot = menu.slot;
			State_getPath(save_path);
			state_slot = last_slot;
			sprintf(bmp_path, "%s/%s.%d.bmp", minui_dir, game.name, menu.slot);
			sprintf(txt_path, "%s/%s.%d.txt", minui_dir, game.name, menu.slot);
		
			save_exists = exists(save_path);
			preview_exists = save_exists && exists(bmp_path);
			// printf("save_path: %s (%i)\n", save_path, save_exists);
			// printf("bmp_path: %s (%i)\n", bmp_path, preview_exists);
		}
		
		if (PAD_justPressed(BTN_B)) {
			status = STATUS_CONT;
			show_menu = 0;
		}
		else if (PAD_justPressed(BTN_A)) {
			switch(selected) {
				case ITEM_CONT:
				if (total_discs && rom_disc!=disc) {
						status = STATUS_DISC;
						char* disc_path = disc_paths[disc];
						Game_changeDisc(disc_path);
					}
					else {
						status = STATUS_CONT;
					}
					show_menu = 0;
				break;
				
				case ITEM_SAVE: {
					state_slot = menu.slot;
					State_write();
					status = STATUS_SAVE;
					SDL_Surface* preview = Menu_thumbnail(backing);
					SDL_RWops* out = SDL_RWFromFile(bmp_path, "wb");
					if (total_discs) {
						char* disc_path = disc_paths[disc];
						putFile(txt_path, disc_path + strlen(base_path));
						sprintf(bmp_path, "%s/%s.%d.bmp", minui_dir, game.name, menu.slot);
					}
					SDL_SaveBMP_RW(preview, out, 1);
					SDL_FreeSurface(preview);
					putInt(slot_path, menu.slot);
					show_menu = 0;
				}
				break;
				case ITEM_LOAD: {
					if (save_exists && total_discs) {
						char slot_disc_name[256];
						getFile(txt_path, slot_disc_name, 256);
						char slot_disc_path[256];
						if (slot_disc_name[0]=='/') strcpy(slot_disc_path, slot_disc_name);
						else sprintf(slot_disc_path, "%s%s", base_path, slot_disc_name);
						char* disc_path = disc_paths[disc];
						if (!exactMatch(slot_disc_path, disc_path)) {
							Game_changeDisc(slot_disc_path);
						}
					}
					state_slot = menu.slot;
					State_read();
					status = STATUS_LOAD;
					putInt(slot_path, menu.slot);
					show_menu = 0;
				}
				break;
				case ITEM_OPTS:
					Core_reset(); // TODO: tmp?
					status = STATUS_OPTS;
					show_menu = 0;
				break;
				case ITEM_QUIT:
					status = STATUS_QUIT;
					show_menu = 0;
					quit = 1; // TODO: tmp?
				break;
			}
			if (!show_menu) break;
		}

		POW_update(&dirty, &show_setting, Menu_beforeSleep, Menu_afterSleep);
		
		if (dirty) {
			SDL_BlitSurface(backing, NULL, screen, NULL);
			SDL_BlitSurface(menu.overlay, NULL, screen, NULL);

			int ox, oy;
			int ow = GFX_blitHardwareGroup(screen, show_setting);
			int max_width = SCREEN_WIDTH - SCALE1(PADDING * 2) - ow;
			
			char display_name[256];
			int text_width = GFX_truncateDisplayName(rom_name, display_name, max_width);
			max_width = MIN(max_width, text_width);

			SDL_Surface* text;
			text = TTF_RenderUTF8_Blended(font.large, display_name, COLOR_WHITE);
			GFX_blitPill(ASSET_BLACK_PILL, screen, &(SDL_Rect){
				SCALE1(PADDING),
				SCALE1(PADDING),
				max_width,
				SCALE1(PILL_SIZE)
			});
			SDL_BlitSurface(text, &(SDL_Rect){
				0,
				0,
				max_width-SCALE1(12*2),
				text->h
			}, screen, &(SDL_Rect){
				SCALE1(PADDING+12),
				SCALE1(PADDING+4)
			});
			SDL_FreeSurface(text);
			
			GFX_blitButtonGroup((char*[]){ "POWER","SLEEP", NULL }, screen, 0);
			GFX_blitButtonGroup((char*[]){ "B","BACK", "A","OKAY", NULL }, screen, 1);
			
			// list
			TTF_SizeUTF8(font.large, menu.items[ITEM_CONT], &ow, NULL);
			ow += SCALE1(12*2);
			oy = 35;
			for (int i=0; i<MENU_ITEM_COUNT; i++) {
				char* item = menu.items[i];
				SDL_Color text_color = COLOR_WHITE;
				
				if (i==selected) {
					// disc change
					if (total_discs>1 && i==ITEM_CONT) {				
						GFX_blitPill(ASSET_DARK_GRAY_PILL, screen, &(SDL_Rect){
							SCALE1(PADDING),
							SCALE1(oy + PADDING),
							SCREEN_WIDTH - SCALE1(PADDING * 2),
							SCALE1(PILL_SIZE)
						});
						text = TTF_RenderUTF8_Blended(font.large, disc_name, COLOR_WHITE);
						SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){
							SCREEN_WIDTH - SCALE1(PADDING + 12) - text->w,
							SCALE1(oy + PADDING + 4)
						});
						SDL_FreeSurface(text);
					}
					
					// pill
					GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){
						SCALE1(PADDING),
						SCALE1(oy + PADDING + (i * PILL_SIZE)),
						ow,
						SCALE1(PILL_SIZE)
					});
					text_color = COLOR_BLACK;
					
					// TODO: draw arrow?
				}
				else {
					// shadow
					text = TTF_RenderUTF8_Blended(font.large, item, COLOR_BLACK);
					SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){
						SCALE1(2 + PADDING + 12),
						SCALE1(1 + PADDING + oy + (i * PILL_SIZE) + 4)
					});
					SDL_FreeSurface(text);
				}
				
				// text
				text = TTF_RenderUTF8_Blended(font.large, item, text_color);
				SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){
					SCALE1(PADDING + 12),
					SCALE1(oy + PADDING + (i * PILL_SIZE) + 4)
				});
				SDL_FreeSurface(text);
			}
			
			// slot preview
			if (selected==ITEM_SAVE || selected==ITEM_LOAD) {
				#define WINDOW_RADIUS 4 // TODO: this logic belongs in blitRect?
				// unscaled
				ox = 146;
				oy = 54;
				int hw = SCREEN_WIDTH / 2;
				int hh = SCREEN_HEIGHT / 2;
				
				// preview window
				// GFX_blitWindow(screen, Screen.menu.preview.x, Screen.menu.preview.y, Screen.menu.preview.width, Screen.menu.preview.height, 1);
				
				// window
				GFX_blitRect(ASSET_STATE_BG, screen, &(SDL_Rect){SCALE2(ox-WINDOW_RADIUS,oy-WINDOW_RADIUS),hw+SCALE1(WINDOW_RADIUS*2),hh+SCALE1(WINDOW_RADIUS*3+6)});
				
				if (preview_exists) { // has save, has preview
					SDL_Surface* preview = IMG_Load(bmp_path);
					if (!preview) printf("IMG_Load: %s\n", IMG_GetError());
					SDL_BlitSurface(preview, NULL, screen, &(SDL_Rect){SCALE2(ox,oy)});
					SDL_FreeSurface(preview);
				}
				else {
					SDL_Rect preview_rect = {SCALE2(ox,oy),hw,hh};
					SDL_FillRect(screen, &preview_rect, 0);
					if (save_exists) { // has save but no preview
						GFX_blitMessage("No Preview", screen, &preview_rect);
					}
					else { // no save
						GFX_blitMessage("Empty Slot", screen, &preview_rect);
					}
				}
				
				// pagination
				ox += 24;
				oy += 124;
				for (int i=0; i<MENU_SLOT_COUNT; i++) {
					if (i==menu.slot) {
						GFX_blitAsset(ASSET_PAGE, NULL, screen, &(SDL_Rect){SCALE2(ox+(i*15),oy)});
					}
					else {
						GFX_blitAsset(ASSET_DOT, NULL, screen, &(SDL_Rect){SCALE2(ox+(i*15)+4,oy+2)});
					}
				}
				
				// SDL_BlitSurface(slot_overlay, NULL, screen, &preview_rect);
				// SDL_BlitSurface(slot_dots, NULL, screen, &(SDL_Rect){Screen.menu.slots.x,Screen.menu.slots.y});
				// SDL_BlitSurface(slot_dot_selected, NULL, screen, &(SDL_Rect){Screen.menu.slots.x+(Screen.menu.slots.ox*slot),Screen.menu.slots.y});
			}
	
			GFX_flip(screen);
			dirty = 0;
		}
		else {
			// slow down to 60fps
			uint32_t frame_duration = SDL_GetTicks() - frame_start;
			#define kTargetFrameDuration 17
			if (frame_duration<kTargetFrameDuration) SDL_Delay(kTargetFrameDuration-frame_duration);
		}
	}
	
	PAD_reset();

	GFX_clearAll();
	if (!quit) SDL_BlitSurface(backing, NULL, screen, NULL);
	SDL_FreeSurface(backing);
	GFX_flip(screen);
	
	POW_disableAutosleep();
}

int main(int argc , char* argv[]) {
	char core_path[MAX_PATH];
	char rom_path[MAX_PATH]; 
	char tag_name[MAX_PATH];
	
	strcpy(core_path, argv[1]);
	strcpy(rom_path, argv[2]);
	getEmuName(rom_path, tag_name);
	
	// LOG_info("core_path: %s\n", core_path);
	// LOG_info("rom_path: %s\n", rom_path);
	// LOG_info("tag_name: %s\n", tag_name);
	
	screen = GFX_init(MODE_MENU);
	InitSettings();
	
	Core_open(core_path, tag_name); 		// LOG_info("after Core_open\n");
	Core_init(); 							// LOG_info("after Core_init\n");
	Game_open(rom_path); 					// LOG_info("after Game_open\n");
	Core_load();  							// LOG_info("after Core_load\n");
	SND_init(core.sample_rate, core.fps);	// LOG_info("after SND_init\n");
	
	Menu_init();
	
	State_resume();
	// State_read();							LOG_info("after State_read\n");
	
	POW_disableAutosleep();
	sec_start = SDL_GetTicks();
	while (!quit) {
		GFX_startFrame();
		
		core.run();
		
		if (show_menu) Menu_loop();
		
		if (0) {
			cpu_ticks += 1;
			uint32_t now = SDL_GetTicks();
			if (now - sec_start>=1000) {
				printf("fps: %i (%i)\n", cpu_ticks, fps_ticks);
				sec_start = now;
				cpu_ticks = 0;
				fps_ticks = 0;
			}
		}
	}
	
	Menu_quit();
	
	Game_close();
	Core_unload();

	Core_quit();
	Core_close(); // LOG_info("after Core_close\n");
	
	SDL_FreeSurface(screen);
	GFX_quit();
	
	return EXIT_SUCCESS;
}
