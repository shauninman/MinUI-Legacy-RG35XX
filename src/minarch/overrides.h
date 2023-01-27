#ifndef OVERRIDES_H
#define OVERRIDES_H

typedef struct OptionOverride {
	char* key;
	char* value;
	int disable; // hides from user
} OptionOverride;

typedef struct ButtonMapping { 
	char* name;
	int retro;
	int local;
} ButtonMapping;

// TODO: not strictly overrides anymore...
typedef struct CoreOverrides {
	char* 			core_name;
	OptionOverride* option_overrides;
	ButtonMapping* 	button_mapping;
} CoreOverrides;

#endif
