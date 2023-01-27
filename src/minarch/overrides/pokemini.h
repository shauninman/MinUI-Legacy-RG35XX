#include "overrides.h"

static CoreOverrides pokemini_overrides = {
	.core_name = "pokemini",
	.option_overrides = (OptionOverride[]){
		{"pokemini_palette",		"Old"},
		{"pokemini_piezofilter",	"disabled"},
		{"pokemini_lowpass_filter",	"enabled"},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"UP",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"DOWN",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"LEFT",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"RIGHT",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"A BUTTON",	RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B BUTTON",	RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"C BUTTON",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{"SHAKE",		RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"POWER",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{NULL,0,0},
	},
};