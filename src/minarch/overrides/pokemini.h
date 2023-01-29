#include "overrides.h"

static CoreOverrides pokemini_overrides = {
	.core_name = "pokemini",
	.option_overrides = (OptionOverride[]){
		{"pokemini_video_scale", 	"6x",1},
		{"pokemini_60hz_mode", 		"enabled",1},
		{"pokemini_palette",		"Old"},
		{"pokemini_piezofilter",	"disabled"},
		{"pokemini_lowpass_filter",	"enabled"},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"A Button",	RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B Button",	RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"C Button",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{"Shake",		RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"Power",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{NULL},
	},
};