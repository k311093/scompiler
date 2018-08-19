#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include "intercode.h"

using namespace std;

enum {WHERE_MEMORY = 4, REGISTER_AX = 0, REGISTER_BX = 1, REGISTER_CX = 2, REGISTER_DX = 3};

class CodeGenerator {
	public:
		CodeGenerator(vector<yystack> &code, string _funcName, FILE *outFile);
		void gen_assem();
	private:
		vector<string> asmcodes;
		vector<int> stored_ax;
		vector<int> stored_bx;
		vector<int> stored_cx;
		vector<int> stored_dx;
		map<int, int> storedTmp;
		vector<interCode> code;
		list<int> leaders;
		string funcName;
		string regNames[4];
		FILE *out;
		int declBlock;
		int variables;
		int tmpVars;
		int curGenerating;

		void output_command(string cmd);
		void generateCode();
		void generatePartial(int start, int end);
		int getRegister(int op, int start, int genLoadCmd);
		int getRegisterExclude(int exclude, int op, int start, int genLoadCmd);
		int getRegister(int registerNo, int op,int start, int genLoadCmd);
		void flush_register(int registerNo, int start);
		void flush_register(int registerNo);
		int next_use(int op,int start);
		void addStored(int reg, int op);
		string operandStr(int op);
		string getJmpCode(int command, int inverse);

};
