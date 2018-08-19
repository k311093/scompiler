#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include "common.h"
#include "intercode.h"
#include "codegen.h"
#include "optimizer.h"

using namespace std;

char cmdName[][30] = {"add", "sub", "if", "goto", "uminus", "mul", "assign", "div", "shift", "logical or", "logical and", "%",
					  "variable","or","and","xor","equal","not equal","less","greater","less=","greater=","shl","shr","inc",
					  "dec","addr","refrence","plus","not","logic not","ifnot","numLiteral","global","return","parameter","argument","call",
					  "callvar","strliteral"};

typedef struct {
	string name;
	void *code;
} funcBody;

vector<funcBody> codeList;

void gen_assem(vector<yystack> &code, FILE *fp, string fname);

extern "C" void insertFunctionBody(char *name, void *bodyCode) {
	funcBody tmp;
	tmp.name = name;
	tmp.code = bodyCode;

	codeList.push_back(tmp);

	return;
}

extern "C" void printFunctionBody(FILE *out, FILE *intcode) {
	int i,j;
	int prevExec, curExec;
	vector<yystack> tmp;

	for(i=0;i<codeList.size();i++) {
		fprintf(out, "%s:\n",codeList[i].name.data());
		fprintf(intcode, "function %s\n",codeList[i].name.data());
		tmp = *((vector<yystack> *)codeList[i].code);
		
		for(j=0;j<tmp.size();j++) {
			fprintf(intcode, "\t;");
			fprintf(intcode,"%4d : ",tmp[j].u_s.intcode.lineNo);
			fprintf(intcode,"%10s\t", cmdName[tmp[j].u_s.intcode.command]);
			if(tmp[j].u_s.intcode.op1 >= 0) {
				fprintf(intcode,"%d\t", tmp[j].u_s.intcode.op1);
			}
			else {
				fprintf(intcode,"-\t");
			}
			if(tmp[j].u_s.intcode.op2 >= 0) {
				fprintf(intcode,"%d\n", tmp[j].u_s.intcode.op2);
			}
			else {
				fprintf(intcode,"-\n");
			}
		}
		
		Optimizer opt(tmp);

		opt.constantPropagate();
		opt.deleteUnreachable();
		opt.deleteDeadCode();

		tmp = opt.getResult();

		prevExec = -1;
		curExec = getExecCount(tmp);
		
		while(prevExec != curExec) {
			prevExec = curExec;

			Optimizer opt2(tmp);

			opt2.constantPropagate();
			opt2.deleteUnreachable();
			opt2.deleteDeadCode();

			tmp = opt2.getResult();
			curExec = getExecCount(tmp);
		}


		fprintf(intcode,"\n\n========== Optimized Code ==========\n");

		for(j=0;j<tmp.size();j++) {
			fprintf(intcode, "\t;");
			fprintf(intcode,"%4d : ",tmp[j].u_s.intcode.lineNo);
			fprintf(intcode,"%10s\t", cmdName[tmp[j].u_s.intcode.command]);
			if(tmp[j].u_s.intcode.op1 >= 0) {
				fprintf(intcode,"%d\t", tmp[j].u_s.intcode.op1);
			}
			else {
				fprintf(intcode,"-\t");
			}
			if(tmp[j].u_s.intcode.op2 >= 0) {
				fprintf(intcode,"%d\n", tmp[j].u_s.intcode.op2);
			}
			else {
				fprintf(intcode,"-\n");
			}
		}

		CodeGenerator gen(tmp , codeList[i].name.data(), out);
		gen.gen_assem();
		
		fprintf(out,"\n");
		fprintf(intcode,"\n");
	}
}
