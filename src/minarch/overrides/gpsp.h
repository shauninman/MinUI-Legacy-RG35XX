#include "overrides.h"

static CoreOverrides gpsp_overrides = {
	.core_name = "gpsp",
	.option_overrides = (OptionOverride[]){
		{"gpsp_save_method", "libretro", 1},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"Start",		RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"A Button",	RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B Button",	RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"A Turbo",		RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_NONE},
		{"B Turbo",		RETRO_DEVICE_ID_JOYPAD_Y,		BTN_ID_NONE},
		{"L Button",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"R Button",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{NULL,0,0},
	},
};