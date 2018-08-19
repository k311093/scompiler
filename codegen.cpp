#include "common.h"
#include "intercode.h"
#include "codegen.h"

using namespace std;

extern "C" char *getFunctionName(int num);

#define EXCLUDE_AX 1
#define EXCLUDE_BX 2
#define EXCLUDE_CX 4
#define EXCLUDE_DX 8

CodeGenerator::CodeGenerator(vector<yystack> &_code, string _funcName, FILE *outFile) {
	int i;
	
	regNames[0] = "eax";
	regNames[1] = "ebx";
	regNames[2] = "ecx";
	regNames[3] = "edx";

	declBlock = 0;
	variables = 0;
	tmpVars = 0;

	out = outFile;
	funcName = _funcName;

	for(i=0;i<_code.size();i++) {
		code.push_back(_code[i].u_s.intcode);
	}

}

void CodeGenerator::output_command(string cmd) {
	char tmpStr[80];
	sprintf(tmpStr, "\t%s\n",cmd.data());
	asmcodes.push_back(tmpStr);
}


void CodeGenerator::gen_assem() {
	int i = 0;
	int parameters = 0;
	char tmpStr[50];

	leaders.clear();

	while(1) {
		if(code[i].command == COMMAND_VARIABLE
		  || code[i].command == COMMAND_GLOBAL
		  || code[i].command == COMMAND_PARAMETER
		  || code[i].command == COMMAND_NUMLIT
		  || code[i].command == COMMAND_STRLIT) {
			if(code[i].command == COMMAND_VARIABLE) {
				if(variables < code[i].op1+1) {
					variables = code[i].op1 + 1;
				}
			}
			i++;
		}
		else break;
	}


	declBlock = i;
	if(i == code.size()) {
		return;
	}

	leaders.push_back(declBlock);

	for(i = declBlock; i<code.size();i++) {
		if(code[i].command == COMMAND_GOTO) {
			leaders.push_back(code[i].op1);	
			leaders.push_back(i+1);
		}
		if(code[i].command == COMMAND_IF ||
			code[i].command == COMMAND_IFNOT) {
			leaders.push_back(code[i].op2);	
			leaders.push_back(i+1);
		}
	}

	leaders.sort();
	leaders.unique();

	generateCode();

	fprintf(out,"\tpush ebp\n");
	fprintf(out,"\tmov ebp, esp\n");

	fprintf(out, "\tsub esp, %d\n", 4*variables+4*tmpVars);
	
	for(i=0;i<asmcodes.size();i++) {
		fprintf(out, "%s",asmcodes[i].data());
	}

	fprintf(out,"  $%s_return:\n",funcName.data());
	fprintf(out,"\tmov esp, ebp\n");
	fprintf(out,"\tpop ebp\n");

	if(parameters == 0) {
		fprintf(out,"\tret\n");
	}
	else {
		fprintf(out, "\tret %d\n", 4*parameters);
	}
	return;
}

void CodeGenerator::generateCode() {
	char tmpStr[256];
	
	int i;
	int start,end;

	for(list<int>::iterator it = leaders.begin();;) {
		start = *(it);
		end = *(++it);
		if(it == leaders.end()) break;
		sprintf(tmpStr,"\n  $%s_%d:",funcName.data(),start);
		output_command(tmpStr);
		generatePartial(start,end);	
	}
	sprintf(tmpStr,"\n  $%s_%d:",funcName.data(),start);
	output_command(tmpStr);
	generatePartial(start,code.size());	

}

string CodeGenerator::operandStr(int op) {
	int i;
	char tmpNum[30];
	int regAssign;

	for(i=0;i<stored_ax.size();i++) {
		if(stored_ax[i] == op) {
			return "eax";
		}
	}

	for(i=0;i<stored_bx.size();i++) {
		if(stored_bx[i] == op) {
			return "ebx";
		}
	}

	for(i=0;i<stored_cx.size();i++) {
		if(stored_cx[i] == op) {
			return "ecx";
		}
	}

	for(i=0;i<stored_dx.size();i++) {
		if(stored_dx[i] == op) {
			return "edx";
		}
	}

	if(code[op].command == COMMAND_VARIABLE) {
		string tmp = "dword [ebp-";
		sprintf(tmpNum,"%d",4+4*code[op].op1);
		return tmp + tmpNum + "]";
	}
	else if(code[op].command == COMMAND_NUMLIT) {
		string tmp = "dword [_num_lit";
		sprintf(tmpNum,"%d",code[op].op1);
		return tmp + tmpNum + "]";

	}
	else if(code[op].command == COMMAND_STRLIT) {
		string tmp = "_str_lit";
		sprintf(tmpNum, "%d", code[op].op1);
		return tmp+tmpNum;
		
	}
	else if(code[op].command == COMMAND_PARAMETER) {
		string tmp = "dword [ebp+";
		sprintf(tmpNum, "%d", 8+code[op].op1*4);
		return tmp+tmpNum+"]";
	}
	
	else if(code[op].command == COMMAND_REF) {
		regAssign = getRegister(code[op].op1,op,1);
		return "dword [" + regNames[regAssign] + "]";
	}
	
	else {
		if(storedTmp.find(op) != storedTmp.end()) {
			sprintf(tmpNum,"dword [ebp-%d]", 8+4*variables+4*storedTmp[op]);
			return tmpNum;
		}
		else {
			return "INVALID_OPERAND";
		}
	}


}

void CodeGenerator::flush_register(int reg) {
	int i;
	vector<int> tmpReg;
	string regname = regNames[reg];
	int regAssign;

	switch(reg) {
		case REGISTER_AX:
			tmpReg = stored_ax;
			stored_ax.clear();
			break;
		case REGISTER_BX:
			tmpReg = stored_bx;
			stored_bx.clear();
			break;
		case REGISTER_CX:
			tmpReg = stored_cx;
			stored_cx.clear();
			break;
		case REGISTER_DX:
			tmpReg = stored_dx;
			stored_dx.clear();
			break;
	}

	for(i=0;i<tmpReg.size();i++) {
		if(code[tmpReg[i]].command == COMMAND_GLOBAL) {
			output_command("mov " + operandStr(tmpReg[i]) + ", " + regname);
		}
	}
}


void CodeGenerator::flush_register(int reg, int start) {
	int i;
	char tmpStr[50];
	vector<int> tmpReg;
	string regname = regNames[reg];
	int regAssign;

	switch(reg) {
		case REGISTER_AX:
			tmpReg = stored_ax;
			stored_ax.clear();
			break;
		case REGISTER_BX:
			tmpReg = stored_bx;
			stored_bx.clear();
			break;
		case REGISTER_CX:
			tmpReg = stored_cx;
			stored_cx.clear();
			break;
		case REGISTER_DX:
			tmpReg = stored_dx;
			stored_dx.clear();
			break;
	}

	for(i=0;i<tmpReg.size();i++) {
		if(!next_use(tmpReg[i], start)) {
			continue;
		}
		if(code[tmpReg[i]].command == COMMAND_STRLIT || code[tmpReg[i]].command == COMMAND_NUMLIT || code[tmpReg[i]].command == COMMAND_REF) {
			continue;
		}
		if((code[tmpReg[i]].command == COMMAND_VARIABLE || 
		  code[tmpReg[i]].command == COMMAND_PARAMETER || code[tmpReg[i]].command == COMMAND_GLOBAL)) {
			output_command("mov " + operandStr(tmpReg[i]) + ", " + regname);
		}
		else {
			if(storedTmp.find(tmpReg[i]) == storedTmp.end()) {
				storedTmp[tmpReg[i]] = tmpVars++;
				sprintf(tmpStr,"mov dword [ebp-%d], %s", 8+4*variables+4*storedTmp[tmpReg[i]], regname.data());
				output_command(tmpStr);
			}
			else {
				sprintf(tmpStr,"mov dword [ebp-%d], %s", 8+4*variables+4*storedTmp[tmpReg[i]], regname.data());
				output_command(tmpStr);
			}
		}
	}
}

int CodeGenerator::getRegister(int registerNo, int op, int start, int genLoadCmd) {
	int i;
	vector<int> tmpReg;

	switch(registerNo) {
		case REGISTER_AX:
			tmpReg = stored_ax;
			break;
		case REGISTER_BX:
			tmpReg = stored_bx;
			break;
		case REGISTER_CX:
			tmpReg = stored_cx;
			break;
		case REGISTER_DX:
			tmpReg = stored_dx;
			break;
	}

	for(i=0;i<tmpReg.size();i++) {
		if(tmpReg[i] == op) {
			return REGISTER_AX;
		}
	}

	flush_register(registerNo, start);
	if(genLoadCmd)
		output_command("mov "+regNames[registerNo] + ", " + operandStr(op));
	addStored(registerNo, op);

	return registerNo;
}

int CodeGenerator::getRegister(int op, int start, int genLoadCmd) {
	int result;
	int minval;
	int i;

	for(i=0;i<stored_ax.size();i++) {
		if(stored_ax[i] == op) {
			return REGISTER_AX;
		}
	}

	for(i=0;i<stored_bx.size();i++) {
		if(stored_bx[i] == op) {
			return REGISTER_BX;
		}
	}

	for(i=0;i<stored_cx.size();i++) {
		if(stored_cx[i] == op) {
			return REGISTER_CX;
		}
	}

	for(i=0;i<stored_dx.size();i++) {
		if(stored_dx[i] == op) {
			return REGISTER_DX;
		}
	}

	result = REGISTER_AX;
	minval = stored_ax.size();

	if(minval > stored_bx.size()) {
		result = REGISTER_BX;
		minval = stored_bx.size();
	}
	if(minval > stored_cx.size()) {
		result = REGISTER_CX;
		minval = stored_cx.size();
	}
	if(minval > stored_dx.size()) {
		result = REGISTER_DX;
		minval = stored_dx.size();
	}

	string name = regNames[result];

	flush_register(result, start);
	if(genLoadCmd)
		output_command("mov " + name + ", " + operandStr(op));
	addStored(result, op);

	return result;
}

int CodeGenerator::getRegisterExclude(int exclude, int op, int start, int genLoadCmd) {
	int result;
	int minval;
	int i;

	if(!(exclude & EXCLUDE_AX)) {
		for(i=0;i<stored_ax.size();i++) {
			if(stored_ax[i] == op) {
				return REGISTER_AX;
			}
		}
	}

	if(!(exclude & EXCLUDE_BX)) {
		for(i=0;i<stored_bx.size();i++) {
			if(stored_bx[i] == op) {
				return REGISTER_BX;
			}
		}
	}

	if(!(exclude & EXCLUDE_CX)) {
		for(i=0;i<stored_cx.size();i++) {
			if(stored_cx[i] == op) {
				return REGISTER_CX;
			}
		}
	}

	if(!(exclude & EXCLUDE_DX)) {
		for(i=0;i<stored_dx.size();i++) {
			if(stored_dx[i] == op) {
				return REGISTER_DX;
			}
		}
	}

	if(!(exclude & EXCLUDE_AX)) {
		result = REGISTER_AX;
		minval = stored_ax.size();
	}
	else if(!(exclude & EXCLUDE_BX)){
		result = REGISTER_BX;
		minval = stored_bx.size();
	}
	else if(!(exclude & EXCLUDE_CX)){
		result = REGISTER_CX;
		minval = stored_bx.size();
	}
	else {
		result = REGISTER_DX;
		minval = stored_bx.size();
	}

	if(!(exclude & EXCLUDE_BX) && minval > stored_bx.size()) {
		result = REGISTER_BX;
		minval = stored_bx.size();
	}
	if(!(exclude & EXCLUDE_CX) && minval > stored_cx.size()) {
		result = REGISTER_CX;
		minval = stored_cx.size();
	}
	if(!(exclude & EXCLUDE_DX) && minval > stored_dx.size()) {
		result = REGISTER_DX;
		minval = stored_dx.size();
	}

	string name = regNames[result];

	flush_register(result, start );
	if(genLoadCmd)
		output_command("mov " + name + ", " + operandStr(op));
	addStored(result, op);

	return result;
}

void CodeGenerator::addStored(int reg, int op) {
	int i;

	switch(reg) {
		case REGISTER_AX:
			for(i=0;i<stored_ax.size();i++) {
				if(stored_ax[i] == op) return;
			}
			stored_ax.push_back(op);
			break;
		case REGISTER_BX:
			for(i=0;i<stored_bx.size();i++) {
				if(stored_bx[i] == op) return;
			}
			stored_bx.push_back(op);
			break;
		case REGISTER_CX:
			for(i=0;i<stored_cx.size();i++) {
				if(stored_cx[i] == op) return;
			}
			stored_cx.push_back(op);
			break;
		case REGISTER_DX:
			for(i=0;i<stored_dx.size();i++) {
				if(stored_dx[i] == op) return;
			}
			stored_dx.push_back(op);
			break;
	}
}

void CodeGenerator::generatePartial(int start, int end) {
	int i;
	int regAssign, regAssign2;
	char tmpStr[255];
	string tmp;
	string assembl;

	for(i=start;i<end;i++) {
		curGenerating = i;
		switch(code[i].command) {
			case COMMAND_ADD:
				regAssign = getRegister(code[i].op1,i,1);
				tmp = regNames[regAssign];
				flush_register(regAssign,i);
				assembl = "add " + tmp + ", " + operandStr(code[i].op2);
				addStored(regAssign, i);
				output_command(assembl);
				break;
			case COMMAND_SUB:
				regAssign = getRegister(code[i].op1,i,1);
				tmp = regNames[regAssign];
				flush_register(regAssign,i);
				assembl = "sub " + tmp + ", " + operandStr(code[i].op2);
				addStored(regAssign, i);
				output_command(assembl);
				break;
			case COMMAND_DIV:
				regAssign = getRegisterExclude(EXCLUDE_AX | EXCLUDE_DX, code[i].op2,i,1);
				getRegister(REGISTER_AX,code[i].op1,i,1);
				flush_register(REGISTER_AX,i);
				flush_register(REGISTER_DX,i);
				output_command("xor edx,edx");
				sprintf(tmpStr, "div %s", regNames[regAssign].data());
				output_command(tmpStr);
				addStored(REGISTER_AX, i);
				break;
			case COMMAND_RES:
				regAssign = getRegisterExclude(EXCLUDE_AX | EXCLUDE_DX, code[i].op2,i,1);
				getRegister(REGISTER_AX,code[i].op1,i,1);
				flush_register(REGISTER_AX,i);
				flush_register(REGISTER_DX,i);
				output_command("xor edx,edx");
				sprintf(tmpStr, "div %s", regNames[regAssign].data());
				output_command(tmpStr);
				addStored(REGISTER_DX, i);
				break;
			case COMMAND_MUL:
				regAssign = getRegisterExclude(EXCLUDE_AX , code[i].op2,i,1);
				getRegister(REGISTER_AX,code[i].op1,i,1);
				flush_register(REGISTER_AX,i);
				sprintf(tmpStr, "mul %s", regNames[regAssign].data());
				output_command(tmpStr);
				addStored(REGISTER_AX, i);
				break;
			case COMMAND_ASSIGN:
				regAssign = getRegister(code[i].op2,i,1);
				if(code[code[i].op1].command == COMMAND_REF) {
					regAssign2 = getRegisterExclude(1>>regAssign, code[code[i].op1].op1 ,i,1);
					output_command("mov [" + regNames[regAssign2] + "] , " + regNames[regAssign]);
				}
				else {
					addStored(regAssign, code[i].op1);
				}
				break;
			case COMMAND_ARGUMENT:
				if(code[code[i].op1].command == COMMAND_STRLIT) {
					sprintf(tmpStr, "%d", code[code[i].op1].op1);
					tmp = tmpStr;
					assembl = "push _str_lit" + tmp;
				}
				else if(code[code[i].op1].command == COMMAND_NUMLIT) {
					sprintf(tmpStr, "%d", code[code[i].op1].op1);
					tmp = tmpStr;
					assembl = "push dword [_num_lit" + tmp + "]";
				}
				else {
					assembl = "push " + operandStr(code[i].op1);
				}
				output_command(assembl);
				break;
			case COMMAND_CALL:
				flush_register(REGISTER_AX,i);
				flush_register(REGISTER_BX,i);
				flush_register(REGISTER_CX,i);
				flush_register(REGISTER_DX,i);
				tmp = getFunctionName(code[i].op1);
				assembl = "call " + tmp;
				output_command(assembl);
				addStored(REGISTER_AX, i);
				break;
			case COMMAND_CALLVAR:
				tmp = getFunctionName(code[i].op1);
				assembl = "call " + tmp;
				output_command(assembl);
				sprintf(tmpStr, "%d", 4*code[i].op2);
				tmp = tmpStr;
				assembl = "add esp, " + tmp;
				flush_register(REGISTER_AX,i);
				output_command(assembl);
				addStored(REGISTER_AX, i);
				break;
			case COMMAND_RETURN:
				if(code[i].op1 != -1) {
					if(operandStr(code[i].op1) != "eax") {
						flush_register(REGISTER_AX);
						sprintf(tmpStr, "mov eax, %s", operandStr(code[i].op1).data());
						flush_register(REGISTER_BX);
						flush_register(REGISTER_CX);
						flush_register(REGISTER_DX);
						output_command(tmpStr);
					}
					if(i != code.size()-1) {
						sprintf(tmpStr, "jmp $%s_return", funcName.data());
						output_command(tmpStr);
					}
				}
				break;
			case COMMAND_EQUAL:
			case COMMAND_NEQUAL:
			case COMMAND_LESS:
			case COMMAND_GR:
			case COMMAND_LESSEQU:
			case COMMAND_GREQU:
				regAssign = getRegister(code[i].op1,i,1);
				sprintf(tmpStr, "cmp %s, %s", regNames[regAssign].data(), operandStr(code[i].op2).data());
				output_command(tmpStr);
				break;	
			case COMMAND_IF:
				if(getJmpCode(code[code[i].op1].command,0) != "") {
					sprintf(tmpStr, "%s $%s_%d", getJmpCode(code[code[i].op1].command, 0).data(),funcName.data(),code[i].op2);
					flush_register(REGISTER_AX,i);
					flush_register(REGISTER_BX,i);
					flush_register(REGISTER_CX,i);
					flush_register(REGISTER_DX,i);
					output_command(tmpStr);
				}
				else {
					regAssign = getRegister(code[i].op1,i,1);
					sprintf(tmpStr,"or %s, %s", regNames[regAssign].data(), regNames[regAssign].data());
					output_command(tmpStr);
					sprintf(tmpStr, "jnz $%s_%d", funcName.data(),code[i].op2);
					flush_register(REGISTER_AX,i);
					flush_register(REGISTER_BX,i);
					flush_register(REGISTER_CX,i);
					flush_register(REGISTER_DX,i);
					output_command(tmpStr);
				}
				break;
			case COMMAND_IFNOT:
				if(getJmpCode(code[code[i].op1].command,0) != "") {
					sprintf(tmpStr, "%s $%s_%d", getJmpCode(code[code[i].op1].command, 1).data(),funcName.data(),code[i].op2);
					flush_register(REGISTER_AX,i);
					flush_register(REGISTER_BX,i);
					flush_register(REGISTER_CX,i);
					flush_register(REGISTER_DX,i);
					output_command(tmpStr);
				}
				else {
					regAssign = getRegister(code[i].op1,i,1);
					sprintf(tmpStr,"or %s, %s", regNames[regAssign].data(), regNames[regAssign].data());
					output_command(tmpStr);
					sprintf(tmpStr, "jz $%s_%d", funcName.data(),code[i].op2);
					flush_register(REGISTER_AX,i);
					flush_register(REGISTER_BX,i);
					flush_register(REGISTER_CX,i);
					flush_register(REGISTER_DX,i);
					output_command(tmpStr);
				}
				break;
			case COMMAND_GOTO:
				sprintf(tmpStr, "jmp $%s_%d", funcName.data(),code[i].op1);
				flush_register(REGISTER_AX,i);
				flush_register(REGISTER_BX,i);
				flush_register(REGISTER_CX,i);
				flush_register(REGISTER_DX,i);
				output_command(tmpStr);
				break;
			case COMMAND_INC:
				regAssign = getRegister(code[i].op1,i,1);
				sprintf(tmpStr, "inc %s", regNames[regAssign].data());
				output_command(tmpStr);
				break;
			case COMMAND_DEC:
				regAssign = getRegister(code[i].op1,i,1);
				sprintf(tmpStr, "dec %s", regNames[regAssign].data());
				output_command(tmpStr);
				break;
			case COMMAND_ADDR:
				regAssign = getRegister(i,i,0);
				if(code[code[i].op1].command == COMMAND_VARIABLE) {
					sprintf(tmpStr,"mov %s, ebp", regNames[regAssign].data());
					output_command(tmpStr);
					sprintf(tmpStr,"sub %s, %d", regNames[regAssign].data(), 4+4*code[code[i].op1].op1);
					output_command(tmpStr);
				}
				break;
		}
	}
	flush_register(REGISTER_AX,i);
	flush_register(REGISTER_BX,i);
	flush_register(REGISTER_CX,i);
	flush_register(REGISTER_DX,i);
}

string CodeGenerator::getJmpCode(int command, int inverse) {
	switch(command) {
		case COMMAND_EQUAL:
			if(inverse) return "jne";
			return "je";
			break;
		case COMMAND_NEQUAL:
			if(inverse) return "je";
			return "jne";
			break;
		case COMMAND_LESS:
			if(inverse) return "jnl";
			return "jl";
			break;
		case COMMAND_GR:
			if(inverse) return "jng";
			return "jg";
			break;
		case COMMAND_LESSEQU:
			if(inverse) return "jg";
			return "jng";
			break;
		case COMMAND_GREQU:
			if(inverse) return "jl";
			return "jnl";
			break;
	}

	return "";
}

int CodeGenerator::next_use(int op, int start) {
	int i;
	int end = code.size();

	int minLabel = start;
	int prevMin = end;
	while(minLabel != prevMin) {
		prevMin = minLabel;
		for(i=minLabel; i < end; i++) {
			if(code[i].command == COMMAND_GOTO) {
				if(minLabel > code[i].op1) {
					minLabel = code[i].op1;
				}
			}
			else if(code[i].command == COMMAND_IF || code[i].command == COMMAND_IFNOT) {
				if(minLabel > code[i].op2) {
					minLabel = code[i].op2;
				}
			}
		}
	}

	for(i=minLabel;i<end;i++) {
		if(code[i].command == COMMAND_IF || code[i].command == COMMAND_IFNOT) {
			if(code[i].op1 == op) {
				return TRUE;
			}
		}
		if(code[i].command != COMMAND_GOTO) {
			if(code[i].op1 == op || code[i].op2 == op) {
				return TRUE;
			}
			if(code[code[i].op1].command == COMMAND_REF) {
				if(code[code[i].op1].op1 == op) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}
