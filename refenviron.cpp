#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "common.h"
#include "declspec.h"

using namespace std;

static int varNo = 0;

static vector<map<string,declaration_specifier> > envStack;
static vector<int> offsets;

extern "C" int initEnviron() {
	map<string,declaration_specifier> curEnv;
	envStack.push_back(curEnv);

	return 0;
}

extern "C" int pushEnviron() {
	map<string,declaration_specifier> curEnv;
	envStack.push_back(curEnv);
	offsets.push_back(varNo);
	varNo = 0;
	return 0;
}

extern "C" int popEnviron() {
	envStack.pop_back();
	varNo = offsets.back();
	offsets.pop_back();
	return 0;
}

extern "C" int pushVariable(declaration_specifier arg, const char *name) {
	string strName = name;

	if(envStack.back().find(strName) == envStack.back().end()) {
		arg.offset = varNo;
		envStack.back()[strName] = arg;
		varNo++;
	}
	else {
		return -1;
	}

	return 0;
}

extern "C" declaration_specifier findVariable(const char *name) {
	int i;
	int offset = 0;
	declaration_specifier ret;

	ret.error = 1;

	string strName = name;
	for(i=envStack.size()-1;i>=0;i--) {
		if(envStack[i].find(strName) != envStack[i].end()) {
			ret = envStack[i][strName];
			if(i == 0)
				ret.global = TRUE;
			ret.error = 0;
			return ret;
		}
	}

	return ret;
}
