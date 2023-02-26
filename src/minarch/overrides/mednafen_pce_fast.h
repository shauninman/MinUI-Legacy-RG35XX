#include "overrides.h"

static CoreOverrides mednafen_pce_fast_overrides = {
	.core_name = "mednafen_pce_fast",
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"Run",			RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"I",			RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"II",			RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"III",			RETRO_DEVICE_ID_JOYPAD_Y,		BTN_ID_Y},
		{"IV",			RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_X},
		{"V",			RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"VI",			RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{"Mode",		RETRO_DEVICE_ID_JOYPAD_L2,		BTN_ID_L2},
		{NULL},
	},
};

