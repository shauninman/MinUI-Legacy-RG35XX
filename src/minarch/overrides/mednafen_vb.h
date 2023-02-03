#include "overrides.h"

static CoreOverrides mednafen_vb_overrides = {
	.core_name = "mednafen_vb",
	.option_overrides = (OptionOverride[]){
		{"vb_3dmode", "anaglyph", 1}, // others crash frontend
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"Start",		RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"A",			RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B",			RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"Low Battery",	RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_NONE},
		{"L Button",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"R Button",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{NULL},
	},
};

