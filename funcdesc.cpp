#include "funcdesc.h"
#include <stdio.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <string.h>

using namespace std;

map<string, function_prototype> funcList;
static int function_count = 0;

extern "C" void setFunctionDefined(char *name) {
	string fname = name;
	
	funcList[fname].defined = 1;
}

extern "C" function_prototype getFunctionByName(char *name) {
	string fname = name;
	function_prototype ret;

	ret.error = 1;

	if(funcList.find(fname) != funcList.end()) {
		ret = funcList[fname];
		ret.error = 0;
		return ret;
	}

	return ret;
}

extern "C" char *getFunctionName(int num) {
	static char ret[255];
	map<string, function_prototype>::iterator it;

	for(it = funcList.begin(); it != funcList.end(); it++) {
		if( (*it).second.offset == num) {
			strcpy(ret, (*it).first.data());
			return ret;
		}
	}

	ret[0] = 0;
	return ret;
}

extern "C" int checkArgMatch(void *arg1, void *arg2) {
	vector<yystack> *list1 = (vector<yystack> *)arg1;
	vector<yystack> *list2 = (vector<yystack> *)arg2;
	int i;

	if(list1 == NULL && list2 == NULL) return TRUE;
	if(list1 == NULL && list2 != NULL) return FALSE;
	if(list1 != NULL && list2 == NULL) return FALSE;
	
	if(list1->size() != list2->size()) {
		return FALSE;
	}
	else {
		for(i=0;i<list1->size();i++) {
			if((*list1)[i].u_s.declspec.type != (*list2)[i].u_s.declspec.type ||
				(*list1)[i].u_s.declspec.pointNo != (*list2)[i].u_s.declspec.pointNo) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

extern "C" int checkMatch(const char *name, declaration_specifier retType, void *argList, char is_vararg) {
	string fname;
	function_prototype proto;

	fname = name;

	int i;

	if(funcList.find(fname) == funcList.end()) {
		return FALSE;
	}
	proto = funcList[fname];

	if(proto.is_vararg == is_vararg && proto.returnType.type == retType.type && proto.returnType.pointNo == retType.pointNo) {
		if(checkArgMatch(argList, proto.argList)) {
			return TRUE;
		}
	}

	return FALSE;
}


extern "C" int addFunction(const char *name, declaration_specifier retType, void *argList, char is_vararg) {
	string fname;
	function_prototype proto;
	
	fname = name;
	proto.is_vararg = is_vararg;
	proto.returnType = retType;
	proto.argList = argList;
	proto.defined = 0;

	if(funcList.find(fname) != funcList.end()) {
		if(!checkMatch(fname.data(), retType, argList, is_vararg)) {
			return -1;
		}
		return 0;
	}

	proto.offset = function_count++;

	funcList[fname] = proto;

	return 0;
}

extern "C" void printFunctions(FILE *fp, FILE *intcode) {
	map<string, function_prototype>::iterator it;

	for(it = funcList.begin(); it != funcList.end(); it++) {
		fprintf(intcode, "function[%d] = %s\n", (*it).second.offset, (*it).first.data());
		if((*it).second.defined) {
			fprintf(fp,"global %s\n", (*it).first.data());
		}
		else
			fprintf(fp,"extern %s\n", (*it).first.data());
	}
}
