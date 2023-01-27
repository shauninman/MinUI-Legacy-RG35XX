#include "overrides.h"

static CoreOverrides picodrive_overrides = {
	.core_name = "picodrive",
	.option_overrides = (OptionOverride[]){
		{"picodrive_sound_rate", "41000", 1},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"UP",			RETRO_DEVICE_ID_JOYPAD_UP,		 BTN_ID_UP},
		{"DOWN",		RETRO_DEVICE_ID_JOYPAD_DOWN,	 BTN_ID_DOWN},
		{"LEFT",		RETRO_DEVICE_ID_JOYPAD_LEFT,	 BTN_ID_LEFT},
		{"RIGHT",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	 BTN_ID_RIGHT},
		{"MODE",		RETRO_DEVICE_ID_JOYPAD_SELECT,	 BTN_ID_SELECT},
		{"START",		RETRO_DEVICE_ID_JOYPAD_START,	 BTN_ID_START},
		{"A BUTTON",	RETRO_DEVICE_ID_JOYPAD_Y,		 BTN_ID_Y},
		{"B BUTTON",	RETRO_DEVICE_ID_JOYPAD_B,		 BTN_ID_X},
		{"C BUTTON",	RETRO_DEVICE_ID_JOYPAD_A,		 BTN_ID_A},
		{"X BUTTON",	RETRO_DEVICE_ID_JOYPAD_L,		 BTN_ID_B},
		{"Y BUTTON",	RETRO_DEVICE_ID_JOYPAD_X,		 BTN_ID_L1},
		{"Z BUTTON",	RETRO_DEVICE_ID_JOYPAD_R,		 BTN_ID_R1},
		{NULL,0,0},
	},
};