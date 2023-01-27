#include "overrides.h"

CoreOverrides gambatte_overrides = {
	.core_name = "gambatte",
	.option_overrides = (OptionOverride[]){
		{"gambatte_gb_colorization",		"internal"},
		{"gambatte_gb_internal_palette",	"TWB64 - Pack 1"},
		{"gambatte_gb_palette_twb64_1",		"TWB64 038 - Pokemon mini Ver."},
		{"gambatte_gb_bootloader",			"disabled"},
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"UP",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"DOWN",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"LEFT",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"RIGHT",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"SELECT",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"START",		RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"A BUTTON",	RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B BUTTON",	RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"A TURBO",		RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_NONE},
		{"B TURBO",		RETRO_DEVICE_ID_JOYPAD_Y,		BTN_ID_NONE},
		{"PREV PAL",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_NONE},
		{"NEXT PAL",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_NONE},
		{NULL,0,0},
	},
};