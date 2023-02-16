#include "overrides.h"

static CoreOverrides mgba_overrides = {
	.core_name = "mgba",
	.option_overrides = (OptionOverride[]){
		{"mgba_force_gbp", "OFF", 1}, // doesn't seem to do anything
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
		{"L Turbo",		RETRO_DEVICE_ID_JOYPAD_L2,		BTN_ID_NONE},
		{"R Turbo",		RETRO_DEVICE_ID_JOYPAD_R2,		BTN_ID_NONE},
		{"More Sun",	RETRO_DEVICE_ID_JOYPAD_L3,		BTN_ID_NONE},
		{"Less Sun",	RETRO_DEVICE_ID_JOYPAD_R3,		BTN_ID_NONE},
		{NULL},
	},
};