#ifndef OVERRIDES_H
#define OVERRIDES_H

typedef struct OptionOverride {
	char* key;
	char* value;
	int lock; // prevents changing this value
} OptionOverride;

typedef struct ButtonMapping { 
	char* name;
	int retro;
	int local; // TODO: dislike this name...
	int mod;
	int default_;
} ButtonMapping;

typedef struct CoreOverrides {
	char* 			core_name; // cannot be NULL
	OptionOverride* option_overrides;
	ButtonMapping* 	button_mapping;
} CoreOverrides;

#endif
