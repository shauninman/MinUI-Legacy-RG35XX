#ifndef OVERRIDES_H
#define OVERRIDES_H

typedef struct OptionOverride {
	char* key;
	char* value;
	int disable; // TODO: hide option from user
} OptionOverride;

typedef struct ButtonMapping { 
	char* name;
	int retro;
	int local;
} ButtonMapping;

typedef struct CoreOverrides {
	char* 			core_name; // cannot be NULL
	OptionOverride* option_overrides;
	ButtonMapping* 	button_mapping;
} CoreOverrides;

#endif
