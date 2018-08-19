#pragma once
#include "common.h"

typedef struct {
	int offset;
	unsigned char global;
	unsigned char parameter;
	unsigned char storage_class;
	unsigned int type;
	int error;
	unsigned char qualifier;
	unsigned char pointNo;
} declaration_specifier;
