#include "overrides.h"

static CoreOverrides picodrive_overrides = {
	.core_name = "picodrive",
	.option_overrides = (OptionOverride[]){
		{"picodrive_sound_rate", "41000", 1},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		 BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	 BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	 BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	 BTN_ID_RIGHT},
		{"Mode",		RETRO_DEVICE_ID_JOYPAD_SELECT,	 BTN_ID_SELECT},
		{"Start",		RETRO_DEVICE_ID_JOYPAD_START,	 BTN_ID_START},
		{"A Button",	RETRO_DEVICE_ID_JOYPAD_Y,		 BTN_ID_Y},
		{"B Button",	RETRO_DEVICE_ID_JOYPAD_B,		 BTN_ID_X},
		{"C Button",	RETRO_DEVICE_ID_JOYPAD_A,		 BTN_ID_A},
		{"X Button",	RETRO_DEVICE_ID_JOYPAD_L,		 BTN_ID_B},
		{"Y Button",	RETRO_DEVICE_ID_JOYPAD_X,		 BTN_ID_L1},
		{"Z Button",	RETRO_DEVICE_ID_JOYPAD_R,		 BTN_ID_R1},
		{NULL},
	},
};