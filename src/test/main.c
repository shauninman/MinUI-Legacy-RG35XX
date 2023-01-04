#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "defines.h"
#include "utils.h"
#include "api.h"

///////////////////////////////////////

#include <sys/time.h>

static uint64_t GFX_getTicks(void) {
    uint64_t ret;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ret = (uint64_t)tv.tv_sec * 1000000;
    ret += (uint64_t)tv.tv_usec;

    return ret;
}

int main (int argc, char *argv[]) {
	SDL_Surface* screen = GFX_init();
	PAD_reset();
	
	int quit = 0;
	while (!quit) {
		// unsigned long frame_start = SDL_GetTicks();
		uint64_t frame_start_us = GFX_getTicks();
		
		PAD_poll();
		if (PAD_anyPressed()) break;
		
		// TODO: diagnosing framepacing issues
		static int frame = 0;
		int x = frame * 8;
		
		void* dst;
		
		dst = screen->pixels;
		memset(dst, (frame%2)?0x00:0xff, (SCREEN_HEIGHT * SCREEN_PITCH));
		// memset(dst, 0, (16 * SCREEN_PITCH));
		// for (int y=0; y<16; y++) {
		// 	memset(dst+(8 * 60 * SCREEN_BPP), 0xff, SCREEN_BPP);
		// 	dst += SCREEN_PITCH;
		// }
		//
		// dst = screen->pixels;
		// dst += (x * SCREEN_BPP);
		//
		// for (int y=0; y<16; y++) {
		// 	memset(dst, 0xff, 8 * SCREEN_BPP);
		// 	dst += SCREEN_PITCH;
		// }

		frame += 1;
		if (frame>=60) frame -= 60;
		
		GFX_flip(screen);
		
		// SDL_Delay(500);

		// slow down to 60fps
		// unsigned long frame_duration = SDL_GetTicks() - frame_start;
		uint64_t frame_duration_us = GFX_getTicks() - frame_start_us;

// #define TARGET_FRAME_DURATION 17
#define TARGET_FRAME_DURATION_US 16666
		// if (frame_duration<TARGET_FRAME_DURATION) SDL_Delay(TARGET_FRAME_DURATION-frame_duration);
		if (frame_duration_us<TARGET_FRAME_DURATION_US) usleep(TARGET_FRAME_DURATION_US-frame_duration_us);

	}
	
	SDL_FreeSurface(screen);
	GFX_quit();
}