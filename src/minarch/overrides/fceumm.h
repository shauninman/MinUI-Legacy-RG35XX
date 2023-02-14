#include "overrides.h"

static CoreOverrides fceumm_overrides = {
	.core_name = "fceumm",
	.option_overrides = (OptionOverride[]){
		{"fceumm_sndquality",	"High"},
		{"fceumm_sndvolume",	"10"},
		{"fceumm_turbo_enable", "Player 1"}, // stupidly defaults to None
		{"fceumm_show_adv_system_options","disabled",1}, // doesn't do anything
		{"fceumm_show_adv_sound_options","disabled", 1}, // doesn't do anything
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
		{"Change Disk",	RETRO_DEVICE_ID_JOYPAD_L,		BTN_ID_NONE},
		{"Insert Disk",	RETRO_DEVICE_ID_JOYPAD_R,		BTN_ID_NONE},
		{"Insert Coin",	RETRO_DEVICE_ID_JOYPAD_R2,		BTN_ID_NONE},
		{NULL},
	},
};

