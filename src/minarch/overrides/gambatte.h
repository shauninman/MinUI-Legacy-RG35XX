#include "overrides.h"

CoreOverrides gambatte_overrides = {
	.core_name = "gambatte",
	.option_overrides = (OptionOverride[]){
		{"gambatte_gb_colorization",		"internal"},
		{"gambatte_gb_internal_palette",	"TWB64 - Pack 1"},
		{"gambatte_gb_palette_twb64_1",		"TWB64 038 - Pokemon mini Ver."},
		{"gambatte_gb_bootloader",			"disabled"},
		{"gambatte_audio_resampler",		"sinc", 1}, // alternatives don't work
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",				RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",			RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",			RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",			RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",			RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"Start",			RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"A Button",		RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"B Button",		RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"A Turbo",			RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_NONE},
		{"B Turbo",			RETRO_DEVICE_ID_JOYPAD_Y,		BTN_ID_NONE},
		{"Prev. Palette",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_NONE},
		{"Next Palette",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_NONE},
		{NULL},
	},
};