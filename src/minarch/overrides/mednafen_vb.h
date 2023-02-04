#include "overrides.h"

static CoreOverrides mednafen_vb_overrides = {
	.core_name = "mednafen_vb",
	.option_overrides = (OptionOverride[]){
		{"vb_3dmode", "anaglyph", 1}, // others crash frontend
		{"vb_right_analog_to_digital", "disabled", 1}, // no analog
		{NULL,NULL},
	},
	.button_mapping = (ButtonMapping[]){
		{"L. Up",		RETRO_DEVICE_ID_JOYPAD_UP,		BTN_ID_UP},
		{"L. Down",		RETRO_DEVICE_ID_JOYPAD_DOWN,	BTN_ID_DOWN},
		{"L. Left",		RETRO_DEVICE_ID_JOYPAD_LEFT,	BTN_ID_LEFT},
		{"L. Right",	RETRO_DEVICE_ID_JOYPAD_RIGHT,	BTN_ID_RIGHT},
		{"Select",		RETRO_DEVICE_ID_JOYPAD_SELECT,	BTN_ID_SELECT},		// ALT: L2
		{"Start",		RETRO_DEVICE_ID_JOYPAD_START,	BTN_ID_START},		// ALT: R2
		{"A",			RETRO_DEVICE_ID_JOYPAD_A,		BTN_ID_A},			// ALT: START
		{"B",			RETRO_DEVICE_ID_JOYPAD_B,		BTN_ID_B},			// ALT: SELECT
		{"L Button",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_L1},
		{"R Button",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_R1}, 
		{"Low Battery",	RETRO_DEVICE_ID_JOYPAD_X,		BTN_ID_NONE},
		{"R. Up",		RETRO_DEVICE_ID_JOYPAD_L2,		BTN_ID_NONE},		// ALT: X
		{"R. Down",		RETRO_DEVICE_ID_JOYPAD_L3,		BTN_ID_NONE},		// ALT: B
		{"R. Left",		RETRO_DEVICE_ID_JOYPAD_R2,		BTN_ID_NONE},		// ALT: Y
		{"R. Right",	RETRO_DEVICE_ID_JOYPAD_R3,		BTN_ID_NONE},		// ALT: A
		{NULL},
	},
};

