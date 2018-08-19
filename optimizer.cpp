#include "optimizer.h"

#define MAX(x,y) ((x)>(y)?(x):(y))

extern "C" int numLiteral(int num);
extern vector<int> numLitList;

Optimizer::Optimizer(vector<yystack> &code) {
	int i;
	optimizeInfo tmp;

	codeInfo.clear();

	for(i=0;i<code.size();i++) {
		tmp.intCode = code[i].u_s.intcode;
		tmp.deleted = 0;
		codeInfo.push_back(tmp);
	}

}

int Optimizer::getRefCount() {
	int i;

	for(i=0;i<codeInfo.size();i++) {
		if(!(codeInfo[i].intCode.command == COMMAND_VARIABLE
		  || codeInfo[i].intCode.command == COMMAND_GLOBAL
		  || codeInfo[i].intCode.command == COMMAND_PARAMETER
		  || codeInfo[i].intCode.command == COMMAND_NUMLIT
		  || codeInfo[i].intCode.command == COMMAND_STRLIT
		  || codeInfo[i].intCode.command == COMMAND_VARIABLE)) {
		  break;
		}
	}

	return i;
}

vector<yystack> Optimizer::getResult() {
	vector<yystack> ret;
	yystack tmp;
	int count = 0;
	int i;
	map<int, int> newLabels;

	for(i=0;i<codeInfo.size();i++) {
		if(!codeInfo[i].deleted) {
			newLabels[i] = count++;
		}
	}

	count = 0;
	for(i=0;i<codeInfo.size();i++) {
		if(!codeInfo[i].deleted) {
			if(i >= getRefCount()) {
				if(codeInfo[i].intCode.command == COMMAND_CALL || codeInfo[i].intCode.command == COMMAND_CALLVAR) {;}
				else {
					if(codeInfo[i].intCode.op1 != -1) {
						codeInfo[i].intCode.op1 = newLabels[codeInfo[i].intCode.op1];
					}
					if(codeInfo[i].intCode.op2 != -1) {
						codeInfo[i].intCode.op2 = newLabels[codeInfo[i].intCode.op2];
					}
				}
			}
			tmp.u_s.intcode = codeInfo[i].intCode;
			tmp.u_s.intcode.lineNo = count++;
			ret.push_back(tmp);
		}
	}

	return ret;
}

void Optimizer::replaceOperand(int from, int to) {
	int i;
	for(i=from;i<codeInfo.size();i++) {
		if(codeInfo[i].intCode.command == COMMAND_GOTO) continue;
		if(codeInfo[i].intCode.command == COMMAND_IF || codeInfo[i].intCode.command == COMMAND_IFNOT) {
			if(codeInfo[i].intCode.op1 != -1 && codeInfo[i].intCode.not_modify_op1 == FALSE &&
			  codeInfo[i].intCode.op1 == from) {
				codeInfo[i].intCode.op1 = to;
				if(to < getRefCount()) {
					codeInfo[i].intCode.not_modify_op1 = TRUE;
				}
			}
		}

		if(codeInfo[i].intCode.op1 != -1 && codeInfo[i].intCode.not_modify_op1 == FALSE &&
		  codeInfo[i].intCode.op1 == from) {
		  	codeInfo[i].intCode.op1 = to;
			if(to < getRefCount()) {
				codeInfo[i].intCode.not_modify_op1 = TRUE;
			}
		}
		if(codeInfo[i].intCode.op2 != -1 && codeInfo[i].intCode.not_modify_op2 == FALSE &&
		  codeInfo[i].intCode.op2 == from) {
		  	codeInfo[i].intCode.op2 = to;
			if(to < getRefCount()) {
				codeInfo[i].intCode.not_modify_op2 = TRUE;
			}
		}
	}
}

int Optimizer::insertRef(optimizeInfo info) {
	int i;

	for(i=0;i<getRefCount();i++) {
		if(codeInfo[i].intCode.command == info.intCode.command &&
			codeInfo[i].intCode.op1 == info.intCode.op1 && 
			codeInfo[i].intCode.op2 == info.intCode.op2) {
				return i;
		}
	}

	insertCommand(i,info);

	return i;
}

void Optimizer::insertCommand(int i, optimizeInfo info) {
	int j;

	info.deleted = 0;
	codeInfo.push_back(info);

	for(j=codeInfo.size()-1;j>i;j--) {
		codeInfo[j] = codeInfo[j-1];
	}
	codeInfo[j] = info;
	for(j=j+1;j<codeInfo.size();j++) {
		if(codeInfo[j].intCode.op1 != -1 && codeInfo[j].intCode.not_modify_op1 == FALSE) {
			codeInfo[j].intCode.op1++;
		}
		if(codeInfo[j].intCode.op2 != -1 && codeInfo[j].intCode.not_modify_op2 == FALSE) {
			codeInfo[j].intCode.op2++;
		}
	}
}

void Optimizer::constantPropagate() {
	optimizeInfo tmp;
	int refEnd, point;
	int ariths = 0;
	int i;

	for(i=0;i<codeInfo.size();i++) {
		tmp.deleted = 0;
		if(codeInfo[i].intCode.command == COMMAND_NUMLIT) {
			codeInfo[i].isConstant = TRUE;
			codeInfo[i].constantValue = numLitList[codeInfo[i].intCode.op1];
		}
		else {
			if(codeInfo[i].intCode.command == COMMAND_IF && codeInfo[codeInfo[i].intCode.op1].isConstant) {
				if(codeInfo[codeInfo[i].intCode.op1].constantValue == 0) {
					codeInfo[i].deleted = 1;
				}
				else {
					codeInfo[i].intCode.command = COMMAND_GOTO;
					codeInfo[i].intCode.op1 = codeInfo[i].intCode.op2;
					codeInfo[i].intCode.op2 = -1;
					codeInfo[i].intCode.not_modify_op1 = FALSE;
					codeInfo[i].intCode.not_modify_op2 = TRUE;
				}
			}
			if(codeInfo[i].intCode.command == COMMAND_IFNOT && codeInfo[codeInfo[i].intCode.op1].isConstant) {
				if(codeInfo[codeInfo[i].intCode.op1].constantValue != 0) {
					codeInfo[i].deleted = 1;
				}
				else {
					codeInfo[i].intCode.command = COMMAND_GOTO;
					codeInfo[i].intCode.op1 = codeInfo[i].intCode.op2;
					codeInfo[i].intCode.op2 = -1;
					codeInfo[i].intCode.not_modify_op1 = FALSE;
					codeInfo[i].intCode.not_modify_op2 = TRUE;
				}
			}
			if(codeInfo[i].intCode.op1 != -1 && codeInfo[codeInfo[i].intCode.op1].isConstant &&
				codeInfo[i].intCode.op2 != -1 && codeInfo[codeInfo[i].intCode.op2].isConstant) {
				ariths = 0;
				switch(codeInfo[i].intCode.command) {
					case COMMAND_ADD:
						codeInfo[i].constantValue = 
						  codeInfo[codeInfo[i].intCode.op1].constantValue +
						  codeInfo[codeInfo[i].intCode.op2].constantValue;
						ariths = 1;
						break;
					case COMMAND_SUB:
						codeInfo[i].constantValue = 
						  codeInfo[codeInfo[i].intCode.op1].constantValue -
						  codeInfo[codeInfo[i].intCode.op2].constantValue;
						ariths = 1;
						break;
					case COMMAND_MUL:
						codeInfo[i].constantValue = 
						  codeInfo[codeInfo[i].intCode.op1].constantValue *
						  codeInfo[codeInfo[i].intCode.op2].constantValue;
						ariths = 1;
						break;
					case COMMAND_DIV:
						codeInfo[i].constantValue = 
						  codeInfo[codeInfo[i].intCode.op1].constantValue /
						  codeInfo[codeInfo[i].intCode.op2].constantValue;
						ariths = 1;
						break;
					case COMMAND_RES:
						codeInfo[i].constantValue = 
						  codeInfo[codeInfo[i].intCode.op1].constantValue *
						  codeInfo[codeInfo[i].intCode.op2].constantValue;
						ariths = 1;
						break;
				}
				if(ariths) {
					codeInfo[i].isConstant = TRUE;
					tmp.deleted = 0;
					tmp.intCode.command = COMMAND_NUMLIT;
					tmp.intCode.op1 = numLiteral(codeInfo[i].constantValue);
					tmp.intCode.not_modify_op1 = TRUE;
					tmp.intCode.op2 = -1;
					tmp.intCode.not_modify_op2 = TRUE;
					tmp.isConstant = TRUE;
					tmp.constantValue = codeInfo[i].constantValue;
					point = insertRef(tmp);
					if(point == getRefCount()) {
						i++;
					}
					replaceOperand(i,point);
				}
			}
			else {
				codeInfo[i].isConstant = FALSE;
			}
		}
	}
}

void Optimizer::deleteDeadCode() {
	int deleted = 1;
	int i,j;


	while(deleted != 0) {
		deleted = 0;
		for(i=0;i<codeInfo.size();i++) {
			switch(codeInfo[i].intCode.command) {
				case COMMAND_IF:
				case COMMAND_IFNOT:
				case COMMAND_GOTO:
				case COMMAND_ASSIGN:
				case COMMAND_INC:
				case COMMAND_DEC:
				case COMMAND_RETURN:
				case COMMAND_ARGUMENT:
				case COMMAND_CALL:
				case COMMAND_CALLVAR:
					codeInfo[i].used = 1;
					break;
				default:
					codeInfo[i].used = 0;
					break;
			}
		}

		for(i=getRefCount();i<codeInfo.size();i++) {
			if(codeInfo[i].deleted) continue;
			switch(codeInfo[i].intCode.command) {
				case COMMAND_IF:
				case COMMAND_IFNOT:
					if(codeInfo[i].intCode.op1 != -1) {
						codeInfo[codeInfo[i].intCode.op1].used = 1;
					}
					break;
				case COMMAND_GOTO:
				case COMMAND_CALL:
				case COMMAND_CALLVAR:
					break;
				default:
					if(codeInfo[i].intCode.op1 != -1) {
						codeInfo[codeInfo[i].intCode.op1].used = 1;
					}
					if(codeInfo[i].intCode.op2 != -1) {
						codeInfo[codeInfo[i].intCode.op2].used = 1;
					}
					break;
			}
		}

		for(i=0;i<codeInfo.size();i++) {
			if(codeInfo[i].deleted) continue;
			if(codeInfo[i].used == 0) {
				deleted++;
				codeInfo[i].deleted = 1;
			}
		}

	}

	for(i=0;i<codeInfo.size();i++) {
		if(codeInfo[i].deleted) continue;
		if(codeInfo[i].intCode.command == COMMAND_GOTO) {
			if(codeInfo[codeInfo[i].intCode.op1].deleted) {
				for(j=codeInfo[i].intCode.op1;codeInfo[j].deleted;j++);
				codeInfo[i].intCode.op1 = j;
			}
		}
		if(codeInfo[i].intCode.command == COMMAND_IF || codeInfo[i].intCode.command == COMMAND_IFNOT) {
			if(codeInfo[codeInfo[i].intCode.op2].deleted) {
				for(j=codeInfo[i].intCode.op2;codeInfo[j].deleted;j++);
				codeInfo[i].intCode.op2 = j;
			}
		}
	}
}

list<int> Optimizer::getLeaders() {
	int i=0;
	int declBlock = 0;
	list<int> leaders;

	while(1) {
		if(codeInfo[i].intCode.command == COMMAND_VARIABLE
		  || codeInfo[i].intCode.command == COMMAND_GLOBAL
		  || codeInfo[i].intCode.command == COMMAND_PARAMETER
		  || codeInfo[i].intCode.command == COMMAND_NUMLIT
		  || codeInfo[i].intCode.command == COMMAND_STRLIT 
		  || codeInfo[i].intCode.command == COMMAND_VARIABLE) {
			i++;
		}
		else break;
	}


	declBlock = i;
	if(i == codeInfo.size()) {
		return leaders;
	}

	leaders.push_back(declBlock);

	for(i = declBlock; i<codeInfo.size();i++) {
		if(codeInfo[i].intCode.command == COMMAND_GOTO) {
			leaders.push_back(codeInfo[i].intCode.op1);	
			leaders.push_back(i+1);
		}
		if(codeInfo[i].intCode.command == COMMAND_IF ||
			codeInfo[i].intCode.command == COMMAND_IFNOT) {
			leaders.push_back(codeInfo[i].intCode.op2);	
			leaders.push_back(i+1);
		}
	}

	leaders.sort();
	leaders.unique();

	return leaders;
}

void Optimizer::deleteUnreachable() {
	char *checked;
	int newcheck = 1;
	int i;

	checked = new char[codeInfo.size()];

	for(i=0;i<codeInfo.size();i++) {
		checked[i] = FALSE;
	}

	checked[0] = TRUE;

	while(newcheck != 0) {
		newcheck = 0;
		for(i=0;i<codeInfo.size();i++) {
			if(checked[i]) {
				if(codeInfo[i].intCode.command == COMMAND_GOTO) {
					if(!checked[codeInfo[i].intCode.op1]) newcheck++;
					checked[codeInfo[i].intCode.op1] = TRUE;
				}
				else if(codeInfo[i].intCode.command == COMMAND_IF || codeInfo[i].intCode.command == COMMAND_IFNOT) {
					if(!checked[codeInfo[i].intCode.op2]) newcheck++;
					checked[codeInfo[i].intCode.op2] = TRUE;
					if(!checked[i+1]) newcheck++;
					checked[i+1] = TRUE;
				}
				else if(codeInfo[i].intCode.command == COMMAND_RETURN) {;}
				else {
					if(!checked[i+1]) newcheck++;
					checked[i+1] = TRUE;
				}
			}
		}
	}

	for(i=0;i<codeInfo.size();i++) {
		if(!checked[i]) {
			codeInfo[i].deleted = 1;
		}
	}

	delete [] checked;
}

int getExecCount(vector<yystack> intCode) {
	int i;

	for(i=0;i<intCode.size();i++) {
		if(!(intCode[i].u_s.intcode.command == COMMAND_VARIABLE
		  || intCode[i].u_s.intcode.command == COMMAND_GLOBAL
		  || intCode[i].u_s.intcode.command == COMMAND_PARAMETER
		  || intCode[i].u_s.intcode.command == COMMAND_NUMLIT
		  || intCode[i].u_s.intcode.command == COMMAND_STRLIT
		  || intCode[i].u_s.intcode.command == COMMAND_VARIABLE)) {
		  break;
		}
	}

	return i;
}
