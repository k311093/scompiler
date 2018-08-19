#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include "common.h"
#include "intercode.h"

using namespace std;

int getExecCount(vector<yystack> intCode); 

typedef struct {
	interCode intCode;
	unsigned char isConstant;
	unsigned char used;
	unsigned char deleted;
	unsigned int constantValue;
} optimizeInfo;

class Optimizer {
	public:
		Optimizer(vector<yystack> &code);
		void constantPropagate();
		void deleteDeadCode();
		void deleteUnreachable();
		vector<yystack> getResult();
	private:
		int getRefCount();
		int insertRef(optimizeInfo info);
		void insertCommand(int i, optimizeInfo info);
		void replaceOperand(int from, int to);
		list<int> getLeaders();
		vector<optimizeInfo> codeInfo;
};
