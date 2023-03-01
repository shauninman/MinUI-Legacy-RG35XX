#include "overrides.h"

static CoreOverrides pcsx_rearmed_overrides = {
	.core_name = "pcsx_rearmed",
	.option_overrides = (OptionOverride[]){
		{"pcsx_rearmed_display_internal_fps",		"disabled", 1},	// doesn't appear to do anything
		{"pcsx_rearmed_show_input_settings",		"disabled", 1}, // doesn't appear to do anything
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"Up",			RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"Right",		RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},
		{"Start",		RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},
		{"Circle",		RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},
		{"Cross",		RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},
		{"Triangle",	RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_X},
		{"Square",		RETRO_DEVICE_ID_JOYPAD_Y,		BTN_ID_Y},
		{"L1 Button",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"R1 Button",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1},
		{"L2 Button",	RETRO_DEVICE_ID_JOYPAD_L2,		BTN_ID_L2},
		{"R2 Button",	RETRO_DEVICE_ID_JOYPAD_R2,		BTN_ID_R2},
		{NULL},
	},
};