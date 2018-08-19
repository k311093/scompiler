#pragma once
#include "common.h"
#include "declspec.h"

typedef struct {
	char is_vararg;	
	int offset;
	int error;
	char defined;
	declaration_specifier returnType;
	void *argList;
} function_prototype;
