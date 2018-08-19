%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "variable.h"
#include "intercode.h"
#include "common.h"
#include "funcdesc.h"

#define YYSTYPE yystack

#define POINTER_WIDTH 4

extern FILE *yyin;

static char fileName[256] = "stdin";

void *createList();
void deleteList(void *arg0);
void insertListEnd(void *arg0, yystack arg1);
int getListSize(void *arg0);
yystack getListElement(void *arg0, int num);
yystack *getElementPointer(void *arg, int num);

int initEnviron();
int pushEnviron();
int popEnviron();
int pushVariable(declaration_specifier arg, const char *name);
declaration_specifier findVariable(const char *name);
void *concat_code(void *arg0, void *arg1, int *op1, int *op2);
yystack *getListBack(void *arg0);
int insertRef(void *list, interCode code); 
int insertCommand(yystack stack, int command, int op1, int op2, int nop1, int nop2);
int opType1(yystack *node, yystack arg0, int command);
int opType2(yystack *node, yystack arg0, yystack arg1, int command);
int errMsg(char *msg);
void yyerror(char *msg);


int numLiteral(int num);
void printLiterals(FILE *fp, FILE * lit);

int checkArgMatch(void *arg1, void *arg2);
int checkMatch(const char *, declaration_specifier, void *, char );
int addFunction(char *, declaration_specifier , void *, char );
function_prototype getFunctionByName(char *name);
char *getFunctionName(int num);
void setFunctionDefined(char *name);
void printFunctions(FILE *fp, FILE *intcode);

void insertFunctionBody(char *name, void *bodyCode);
void printFunctionBody(FILE *fp, FILE *intcode);


int i,j;
int op1,op2;
int point1,point2;
int intCodeLine = 0;
interCode code;
extern int lineNum;
void *intCodeList;
yystack tmp_stack,tmp_stack2;
void *refList;
yystack *nodePointer;
int errCount = 0;
int width;


int typeWidth[] = {0,1,2,4,4,8,4,4};

%}

%token STORAGE_CLASS_SPECIFIER TYPE_SPECIFIER TYPE_QUALIFIER ASSIGNMENT_OPERATOR
%token INTEGER_CONSTANT CASE SIZEOF LOGIC_OR GOTO DO WHILE IF FOR ENUM
%token ELSE LOGIC_AND SWITCH NEQUAL BREAK SHR SHL ETC STRUCT LTE DEFAULT
%token STRING MM ENUMERATION_CONSTANT IDENTIFIER CHARACTER_CONSTANT PP CONTINUE UNION
%token GTE RETURN ARROW FLOATING_CONSTANT EQUAL TYPEDEF_NAME
%token DEFINITION_STORAGE_CLASS DEFINITION_TYPE_SPECIFIER

%%
translation_unit:
	external_declaration
	|translation_unit external_declaration
	;
external_declaration:
	function_definition
	|declaration {
	}
declaration:
	declaration_specifiers init_declarator_list_opt ';' {
		$$.list_pointer = $2.list_pointer;
		$$.u_s.declspec = $1.u_s.declspec;
		if($2.list_pointer != NULL) {
			for(i=0;i<getListSize($2.list_pointer);i++) {
				tmp_stack = getListElement($2.list_pointer,i);
				switch(tmp_stack.node_type) {
				case TYPE_VARDECL:
					tmp_stack.u_s.declspec.storage_class = $$.u_s.declspec.storage_class;
					tmp_stack.u_s.declspec.type = $$.u_s.declspec.type;
					tmp_stack.u_s.declspec.qualifier = $$.u_s.declspec.qualifier;
					pushVariable(tmp_stack.u_s.declspec, tmp_stack.name);
					break;
				case TYPE_FUNCDECL:
					$1.u_s.declspec.pointNo = tmp_stack.u_s.declspec.pointNo;
					if(addFunction(tmp_stack.name, $1.u_s.declspec, tmp_stack.list_pointer, tmp_stack.is_vararg)) {
						errMsg("prototype does not match or function already defined");	
					}
					break;
				}
			}
		}
	}
	;
function_definition:
	definition_specifiers_opt func_declarator declaration_list_opt compound_statement	{
		if(getListBack($4.list_pointer)->u_s.intcode.command != COMMAND_RETURN)
			insertCommand($4,COMMAND_RETURN,-1,-1,TRUE,TRUE);
		intCodeList = concat_code(refList,$4.list_pointer,&op1,&op2);
		refList = createList();
		$1.u_s.declspec.pointNo = $2.u_s.declspec.pointNo;
		if(addFunction($2.name, $1.u_s.declspec, $2.list_pointer, $2.is_vararg)) {
				errMsg("prototype does not match or function already defined");	
		}
		setFunctionDefined($2.name);
		insertFunctionBody($2.name, intCodeList);
		intCodeList = createList();
		popEnviron();
	}
	;
definition_specifiers:
	definition_storage_class definition_specifiers_opt {
		$$.u_s.declspec.storage_class =
		$1.u_s.declspec.storage_class | $2.u_s.declspec.storage_class;
		$$.u_s.declspec.type = $2.u_s.declspec.type;
		$$.u_s.declspec.qualifier = $2.u_s.declspec.qualifier;
	}
	|definition_type_specifier definition_specifiers_opt {
		$$.u_s.declspec.type =
		$1.u_s.declspec.type | $2.u_s.declspec.type;
		$$.u_s.declspec.qualifier = $2.u_s.declspec.qualifier;
		$$.u_s.declspec.storage_class = $2.u_s.declspec.storage_class;
	}
	;
definition_storage_class:
	DEFINITION_STORAGE_CLASS {
		SET_BIT($$.u_s.declspec.storage_class,$1.number);
	}
	;
definition_type_specifier:
	DEFINITION_TYPE_SPECIFIER {
		$$.u_s.declspec.type = $1.number;
	}
	;
declaration_list:
	declaration {
		$$.list_pointer = (void *)createList();
		insertListEnd($$.list_pointer, $1);
	}
	|declaration_list declaration  {
		insertListEnd($1.list_pointer, $2);
	}
	;
declaration_specifiers:
	storage_class_specifier declaration_specifiers_opt {
		$$.u_s.declspec.storage_class =
		$1.u_s.declspec.storage_class | $2.u_s.declspec.storage_class;
		$$.u_s.declspec.type = $2.u_s.declspec.type;
		$$.u_s.declspec.qualifier = $2.u_s.declspec.qualifier;
	}
	|type_specifier declaration_specifiers_opt {
		$$.u_s.declspec.type =
		$1.u_s.declspec.type | $2.u_s.declspec.type;
		$$.u_s.declspec.qualifier = $2.u_s.declspec.qualifier;
		$$.u_s.declspec.storage_class = $2.u_s.declspec.storage_class;
	}
	|type_qualifier declaration_specifiers_opt {
		$$.u_s.declspec.qualifier =
		$1.u_s.declspec.qualifier | $2.u_s.declspec.qualifier;
		$$.u_s.declspec.type = $2.u_s.declspec.type;
		$$.u_s.declspec.storage_class = $2.u_s.declspec.storage_class;
	}
	;
storage_class_specifier:
	STORAGE_CLASS_SPECIFIER {
		SET_BIT($$.u_s.declspec.storage_class,$1.number);
	}
	;
type_specifier:
	TYPE_SPECIFIER {
		$$.u_s.declspec.type = $1.number;
	}
	|struct_or_union_specifier
	|enum_specifier
	|typedef_name
	;
type_qualifier:
	TYPE_QUALIFIER {
		SET_BIT($$.u_s.declspec.qualifier,$1.number);
	}
	;
struct_or_union_specifier:
	struct_or_union identifier_opt '{' struct_declaration_list '}'
	|struct_or_union identifier
	;
struct_or_union:
	STRUCT
	|UNION
	;
struct_declaration_list:
	struct_declaration
	|struct_declaration_list struct_declaration
	;
init_declarator_list:
	init_declarator {
		$$.list_pointer = createList();
		insertListEnd($$.list_pointer, $1);
	}
	|init_declarator_list ',' init_declarator {
		insertListEnd($1.list_pointer, $3);
	}
	;
init_declarator:
	declarator {
		$$.name = $1.name;
		$$.is_vararg = $1.is_vararg;
		$$.node_type = $1.node_type;
		$$.u_s.declspec.pointNo = $1.u_s.declspec.pointNo;
		$$.list_pointer = $1.list_pointer;
	}
	|declarator '=' initializer {
	}
	;
struct_declaration:
	specifier_qualifier_list struct_declarator_list ';'
	;
specifier_qualifier_list:
	type_specifier specifier_qualifier_list_opt
	|type_qualifier specifier_qualifier_list_opt
	;
struct_declarator_list:
	struct_declarator
	|struct_declarator_list ',' struct_declarator
	;
struct_declarator:
	declarator
	|declarator_opt ':' constant_expression
	;
enum_specifier:
	ENUM identifier_opt '{' enumerator_list '}'
	|ENUM identifier
	;
enumerator_list:
	enumerator
	|enumerator_list ',' enumerator
	;
enumerator:
	identifier
	|identifier '=' constant_expression
	;
declarator:
	pointer_opt direct_declarator {
		$$.name = $2.name;
		$$.node_type = $2.node_type;
		$$.u_s.declspec.pointNo = $1.u_s.declspec.pointNo;
		$$.list_pointer = $2.list_pointer;
		$$.is_vararg = $2.is_vararg;
	}
	;
func_declarator:
	pointer_opt direct_declarator {
		$$.name = $2.name;
		$$.node_type = $2.node_type;
		$$.u_s.declspec.pointNo = $1.u_s.declspec.pointNo;
		$$.list_pointer = $2.list_pointer;
		$$.is_vararg = $2.is_vararg;
		pushEnviron();
		for(i=0;i<getListSize($$.list_pointer);i++) {
			tmp_stack = getListElement($$.list_pointer,i);
			pushVariable(tmp_stack.u_s.declspec,tmp_stack.name);
		}
	}
	;
direct_declarator:
	identifier {
		$$.name = $1.name;
		$$.node_type = TYPE_VARDECL;
	}
	|'(' declarator ')'
	|direct_declarator '[' constant_expression_opt ']'
	|direct_declarator '(' parameter_type_list ')'	{
		$$.name = $1.name;
		$$.list_pointer = $3.list_pointer;
		$$.node_type = TYPE_FUNCDECL;
		$$.is_vararg = $3.is_vararg;
	}
	|direct_declarator '(' identifier_list_opt ')' {
		$$.name = $1.name;
		$$.list_pointer = $3.list_pointer;
		$$.node_type = TYPE_FUNCDECL;
		$$.node_type = TYPE_FUNCDECL;
		$$.is_vararg = $3.is_vararg;
	}
	;
pointer:
	'*' type_qualifier_list_opt {
		$$.u_s.declspec.pointNo = 1;
	}
	|'*' type_qualifier_list_opt pointer {
		$$.u_s.declspec.pointNo = $3.u_s.declspec.pointNo + 1;
	}
	;
type_qualifier_list:
	type_qualifier
	|type_qualifier_list type_qualifier
	;
parameter_type_list:
	parameter_list {
		$$ = $1;
		$$.is_vararg = FALSE;
	}
	|parameter_list ',' ETC {
		$$ = $1;
		$$.is_vararg = TRUE;
	}
	;
parameter_list:
	parameter_declaration {
		$$.list_pointer = createList();
		insertListEnd($$.list_pointer, $1);
	}
	|parameter_list ',' parameter_declaration {
		insertListEnd($1.list_pointer, $3);
	}
	;
parameter_declaration:
	declaration_specifiers declarator {
		$$.u_s.declspec = $1.u_s.declspec;
		$$.u_s.declspec.pointNo = $2.u_s.declspec.pointNo;
		$$.name = $2.name;
		$$.u_s.declspec.parameter = TRUE;
	}
	|declaration_specifiers abstract_declarator_opt
	;
identifier_list:
	identifier
	|identifier_list ',' identifier
	;
initializer:
	assignment_expression {
	}
	|'{' initializer_list '}' {
	}
	|'{' initializer_list ',' '}'
	;
initializer_list:
	initializer
	|initializer_list ',' initializer
	;
type_name:
	specifier_qualifier_list abstract_declarator_opt
	;
abstract_declarator:
	pointer
	|pointer_opt direct_abstract_declarator
	;
direct_abstract_declarator:
	'(' abstract_declarator ')'
	|direct_abstract_declarator_opt '[' constant_expression_opt ']'
	|direct_abstract_declarator_opt '(' parameter_type_list_opt ')'
	;
typedef_name:
	TYPEDEF_NAME {
	}
	;
statement:
	labeled_statement {$$ = $1; }
	|expression_statement { $$ = $1; }
	|compound_statement { $$ = $1; }
	|selection_statement { $$ = $1; }
	|iteration_statement { $$ = $1; }
	|jump_statement { $$ = $1; }
	;
labeled_statement:
	identifier ':' statement
	|CASE constant_expression ':' statement
	|DEFAULT ':' statement
	;
expression_statement:
	expression_opt ';'
	;
compound_statement:
	compound_start declaration_list_opt statement_list_opt compound_end {
		$$.list_pointer = createList();
		if($3.list_pointer != 0) {
			$$.list_pointer = concat_code($$.list_pointer,$3.list_pointer,&op1,&op2);
		}
	}
	;
compound_start:
	'{' {
		pushEnviron();
	}
compound_end:
	'}' {
		popEnviron();
	}
statement_list:
	statement {
		$$.list_pointer = concat_code(NULL, $1.list_pointer,&op1,&op2);
	}
	|statement_list statement {
		$$.list_pointer = concat_code($1.list_pointer, $2.list_pointer,&op1,&op2);
	}
	;
selection_statement:
	IF '(' expression ')' statement {
		opType1(&$$,$3,COMMAND_IFNOT);
		op2 = getListSize($5.list_pointer)+getListSize($$.list_pointer);
		nodePointer = getListBack($$.list_pointer);
		nodePointer->u_s.intcode.op2 = op2;
		nodePointer->u_s.intcode.not_modify_op2 = FALSE;
		$$.list_pointer = concat_code($$.list_pointer, $5.list_pointer,&op1,&op2);
	}
	|IF '(' expression ')' statement ELSE statement
	|SWITCH '(' expression ')' statement
	;
iteration_statement:
	WHILE '(' expression ')' statement {
		opType1(&$$,$3,COMMAND_IFNOT);
		point1 = getListSize($5.list_pointer)+getListSize($$.list_pointer);
		nodePointer = getListBack($$.list_pointer);
		nodePointer->u_s.intcode.op2 = point1+1;
		nodePointer->u_s.intcode.not_modify_op2 = FALSE;
		$$.list_pointer = concat_code($$.list_pointer, $5.list_pointer,&op1,&op2);
		insertCommand($$,COMMAND_GOTO, 0, OPERATOR_NONE, FALSE, TRUE);
		for(i=0;i<getListSize($$.list_pointer);i++) {
			if(getElementPointer($$.list_pointer,i)->u_s.intcode.op1 == OPERATOR_BREAK) {
				getElementPointer($$.list_pointer,i)->u_s.intcode.op1 = point1+1;
				getElementPointer($$.list_pointer,i)->u_s.intcode.not_modify_op1 = FALSE;
			}
		}
	}
	|DO statement WHILE '(' expression ')' ';'
	|FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement {
		$$.list_pointer = concat_code(NULL, $3.list_pointer,&op1,&op2);
		point1 = op2+1;
		if(!($5.node_type == TYPE_REFRENCE || $5.list_pointer == NULL)) {
			opType1(&tmp_stack,$5,COMMAND_IFNOT);
			$$.list_pointer = concat_code($$.list_pointer, tmp_stack.list_pointer,&op1,&op2);
			point2 = getListSize($9.list_pointer) + getListSize($7.list_pointer) + getListSize($$.list_pointer) + 1;
			nodePointer = getListBack($$.list_pointer);
			nodePointer->u_s.intcode.op2 = point2;
			nodePointer->u_s.intcode.not_modify_op2 = FALSE;
		}
		$$.list_pointer = concat_code($$.list_pointer, $9.list_pointer,&op1,&op2);
		$$.list_pointer = concat_code($$.list_pointer, $7.list_pointer,&op1,&op2);
		insertCommand($$,COMMAND_GOTO, point1, OPERATOR_NONE, FALSE, TRUE);
		for(i=0;i<getListSize($$.list_pointer);i++) {
			if(getElementPointer($$.list_pointer,i)->u_s.intcode.op1 == OPERATOR_BREAK) {
				getElementPointer($$.list_pointer,i)->u_s.intcode.op1 = point2;
				getElementPointer($$.list_pointer,i)->u_s.intcode.not_modify_op1 = FALSE;
			}
		}
	}
	;
jump_statement:
	GOTO identifier ';'
	|CONTINUE ';'
	|BREAK ';' {
		$$.list_pointer = createList();
		insertCommand($$,COMMAND_GOTO, OPERATOR_BREAK, OPERATOR_NONE, TRUE, TRUE);
	}
	|RETURN expression ';' {
		$$.list_pointer = createList();
		opType1(&$$,$2,COMMAND_RETURN);
	}
	;
expression:
	assignment_expression { $$ = $1; }
	|expression ',' assignment_expression { $$.list_pointer = concat_code($1.list_pointer,$3.list_pointer,&op1,&op2); }
	;
assignment_expression:
	conditional_expression { $$ = $1; }
	|unary_expression assignment_operator assignment_expression { 
		if(!strcmp($2.name, "=")) {
			opType2(&$$,$1,$3,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"*=")) {
			opType2(&$$,$1,$3,COMMAND_MUL);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"/=")) {
			opType2(&$$,$1,$3,COMMAND_DIV);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"%=")) {
			opType2(&$$,$1,$3,COMMAND_RES);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"+=")) {
			opType2(&$$,$1,$3,COMMAND_ADD);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"-=")) {
			opType2(&$$,$1,$3,COMMAND_SUB);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"<<=")) {
			opType2(&$$,$1,$3,COMMAND_SHL);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,">>=")) {
			opType2(&$$,$1,$3,COMMAND_SHR);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"&=")) {
			opType2(&$$,$1,$3,COMMAND_AND);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"^=")) {
			opType2(&$$,$1,$3,COMMAND_XOR);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
		else if(!strcmp($2.name,"|=")) {
			opType2(&$$,$1,$3,COMMAND_OR);
			opType2(&$$,$1,$$,COMMAND_ASSIGN);
		}
	}
	;
assignment_operator:
	ASSIGNMENT_OPERATOR { $$.name = $1.name; }
	|'=' { $$.name = $1.name; }
	;
conditional_expression:
	logical_OR_expression { $$ = $1; }
	|logical_OR_expression '?' expression ':' conditional_expression
	;
constant_expression:
	conditional_expression { $$ = $1; }
	;
logical_OR_expression:
	logical_AND_expression { $$ = $1; }
	|logical_OR_expression LOGIC_OR logical_AND_expression { opType2(&$$,$1,$3,COMMAND_LOR); }
	;
logical_AND_expression:
	inclusive_OR_expression { $$ = $1; }
	|logical_AND_expression LOGIC_AND inclusive_OR_expression { opType2(&$$,$1,$3,COMMAND_LAND); }
	;
inclusive_OR_expression:
	exclusive_OR_expression { $$ = $1; }
	|inclusive_OR_expression '|' exclusive_OR_expression { opType2(&$$,$1,$3,COMMAND_OR); }
	;
exclusive_OR_expression:
	AND_expression { $$ = $1; }
	|exclusive_OR_expression '^' AND_expression { opType2(&$$,$1,$3,COMMAND_XOR); }
	;
AND_expression:
	equality_expression { $$ = $1; }
	|AND_expression '&' equality_expression { opType2(&$$,$1,$3,COMMAND_AND); }
	;
equality_expression:
	relational_expression { $$ = $1; }
	|equality_expression EQUAL relational_expression { opType2(&$$,$1,$3,COMMAND_EQUAL); }
	|equality_expression NEQUAL relational_expression { opType2(&$$,$1,$3,COMMAND_NEQUAL); }
	;
relational_expression:
	shift_expression { $$ = $1; }
	|relational_expression '<' shift_expression { opType2(&$$,$1,$3,COMMAND_LESS); }
	|relational_expression '>' shift_expression { opType2(&$$,$1,$3,COMMAND_GR); }
	|relational_expression LTE shift_expression { opType2(&$$,$1,$3,COMMAND_LESSEQU); }
	|relational_expression GTE shift_expression { opType2(&$$,$1,$3,COMMAND_GREQU); }
	;
shift_expression:
	additive_expression { $$ = $1; }
	|shift_expression SHL additive_expression { opType2(&$$,$1,$3,COMMAND_SHL); }
	|shift_expression SHR additive_expression { opType2(&$$,$1,$3,COMMAND_SHR); }
	;
additive_expression:
	multiplicative_expression { $$ = $1; }
	|additive_expression '+' multiplicative_expression { opType2(&$$,$1,$3,COMMAND_ADD); }
	|additive_expression '-' multiplicative_expression { opType2(&$$,$1,$3,COMMAND_SUB); }
	;
multiplicative_expression:
	cast_expression { $$ = $1; }
	|multiplicative_expression '*' cast_expression { opType2(&$$,$1,$3,COMMAND_MUL); }
	|multiplicative_expression '/' cast_expression { opType2(&$$,$1,$3,COMMAND_DIV); }
	|multiplicative_expression '%' cast_expression { opType2(&$$,$1,$3,COMMAND_RES); }
	;
cast_expression:
	unary_expression { $$ = $1; }
	|'(' type_name ')' cast_expression
	;
unary_expression:
	postfix_expression { $$ = $1; }
	|PP unary_expression { opType1(&$$,$2,COMMAND_INC); }
	|MM unary_expression { opType1(&$$,$2,COMMAND_DEC); }
	|unary_operator cast_expression {
		switch($1.number) {
			case '&':
				opType1(&$$,$2,COMMAND_ADDR);
				break;
			case '*':
				opType1(&$$,$2,COMMAND_REF);
				break;
			case '+':
				opType1(&$$,$2,COMMAND_UPLUS);
				break;
			case '-':
				opType1(&$$,$2,COMMAND_UMINUS);
				break;
			case '~':
				opType1(&$$,$2,COMMAND_NOT);
				break;
			case '!':
				opType1(&$$,$2,COMMAND_LNOT);
				break;
		}
	}
	|SIZEOF unary_expression
	|SIZEOF '(' type_name ')'
	;
unary_operator:
	'&' {$$.number = $1.number;}
	|'*' {$$.number = $1.number;}
	|'+' {$$.number = $1.number;}
	|'-' {$$.number = $1.number;}
	|'~' {$$.number = $1.number;}
	|'!' {$$.number = $1.number;}
	;
postfix_expression:
	primary_expression { $$ = $1; }
	|postfix_expression '[' expression ']' {
		if($1.u_s.declspec.pointNo == 0) {
			errMsg("not array or pointer");
		}

		if($1.u_s.declspec.pointNo - 1 != 0) {
			width = 4;
		}
		else {
			width = typeWidth[$1.u_s.declspec.type];
		}
		tmp_stack.u_s.intcode.command = COMMAND_NUMLIT;
		tmp_stack.u_s.intcode.op1 = numLiteral(width);
		tmp_stack.u_s.intcode.op2 = OPERATOR_NONE;
		tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
		tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
		tmp_stack.u_s.intcode.lineNo = 0;
		i = insertRef(refList,tmp_stack.u_s.intcode);
		tmp_stack.number = i;
		tmp_stack.node_type = TYPE_REFRENCE;
		opType2(&$$,$3,tmp_stack,COMMAND_MUL);
		opType2(&$$,$$,$1,COMMAND_ADD);
		insertCommand($$,COMMAND_REF,getListSize($$.list_pointer)-1,OPERATOR_NONE,FALSE,TRUE);
		$$.node_type = TYPE_ELSE;
		$$.u_s.declspec = $1.u_s.declspec;
		$$.u_s.declspec.pointNo--;
	}
	|postfix_expression '(' argument_expression_list_opt ')'	{
		if(!getFunctionByName($1.name).error) {
			$$.list_pointer = createList();
			for(i=getListSize($3.list_pointer)-1;i>=0;i--) {
				opType1(&tmp_stack, getListElement($3.list_pointer,i), COMMAND_ARGUMENT);
				$$.list_pointer = concat_code($$.list_pointer, tmp_stack.list_pointer,&op1,&op2);
			}
			if(getFunctionByName($1.name).is_vararg) {
				insertCommand($$,COMMAND_CALLVAR,getFunctionByName($1.name).offset,getListSize($3.list_pointer),TRUE,TRUE);
			}
			else {
				insertCommand($$,COMMAND_CALL,getFunctionByName($1.name).offset,-1,TRUE,TRUE);
			}
		}
		else {
			errMsg("is not a function");
		}
	}
	|postfix_expression '.' identifier
	|postfix_expression ARROW identifier
	|postfix_expression PP {
		if(!($1.u_s.declspec.type==TYPE_CHAR || $1.u_s.declspec.type==TYPE_SHORT ||
			$1.u_s.declspec.type==TYPE_INT || $1.u_s.declspec.type==TYPE_SIGNED ||
			$1.u_s.declspec.type==TYPE_UNSIGNED || $1.u_s.declspec.pointNo != 0)) {
				errMsg("operator cannot be used with this type\n");
		}
		opType1(&$$,$1,COMMAND_INC);
	}
	|postfix_expression MM { 
		if(!($1.u_s.declspec.type==TYPE_CHAR || $1.u_s.declspec.type==TYPE_SHORT ||
			$1.u_s.declspec.type==TYPE_INT || $1.u_s.declspec.type==TYPE_SIGNED ||
			$1.u_s.declspec.type==TYPE_UNSIGNED || $1.u_s.declspec.pointNo != 0)) {
			errMsg("operator cannot be used with this type\n");
		}
		opType1(&$$,$1,COMMAND_DEC); 
	}
	;
primary_expression:
	identifier {
		$$.name = $1.name;
		if(!findVariable($1.name).error) {
			if(findVariable($1.name).global == TRUE) {
				tmp_stack.u_s.intcode.command = COMMAND_GLOBAL;
			}
			else if(findVariable($1.name).parameter == TRUE) {
				tmp_stack.u_s.intcode.command = COMMAND_PARAMETER;
			}
			else {
				tmp_stack.u_s.intcode.command = COMMAND_VARIABLE;
			}
			tmp_stack.u_s.intcode.op1 = findVariable($1.name).offset;
			tmp_stack.u_s.intcode.op2 = OPERATOR_NONE;
			tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
			tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
			tmp_stack.u_s.intcode.lineNo = 0;
			i = insertRef(refList,tmp_stack.u_s.intcode);
			$$.number = i;
			$$.node_type = TYPE_REFRENCE;
			$$.u_s.declspec = findVariable($1.name);
		}
		else if(getFunctionByName($1.name).error) {
			errMsg("cannot find identifier");
		}
	}
	|constant {
		$$ = $1;
	}
	|string {
		$$ = $1;
	}
	|'(' expression ')' {
		$$ = $2;
	}
	;
argument_expression_list:
	assignment_expression	{
		$$.list_pointer = createList();
		insertListEnd($$.list_pointer, $1);
	}
	|argument_expression_list ',' assignment_expression		{
		insertListEnd($$.list_pointer, $3);
	}
	;
constant:
	integer_constant {
		tmp_stack.u_s.intcode.command = COMMAND_NUMLIT;
		tmp_stack.u_s.intcode.op1 = numLiteral($1.number);
		tmp_stack.u_s.intcode.op2 = OPERATOR_NONE;
		tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
		tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
		tmp_stack.u_s.intcode.lineNo = 0;
		i = insertRef(refList,tmp_stack.u_s.intcode);
		$$.number = i;
		$$.node_type = TYPE_REFRENCE;
	}
	|character_constant
	|floating_constant
	|enumeration_constant
	;
identifier:
	IDENTIFIER { $$.name = $1.name; }
	;
string:
	STRING {
		tmp_stack.u_s.intcode.command = COMMAND_STRLIT;
		tmp_stack.u_s.intcode.op1 = strLiteral($1.name);
		tmp_stack.u_s.intcode.op2 = OPERATOR_NONE;
		tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
		tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
		tmp_stack.u_s.intcode.lineNo = 0;
		i = insertRef(refList,tmp_stack.u_s.intcode);
		$$.number = i;
		$$.node_type = TYPE_REFRENCE;
	}
	;
integer_constant:
	INTEGER_CONSTANT { $$.number = $1.number; }
	;
character_constant:
	CHARACTER_CONSTANT
	;
floating_constant:
	FLOATING_CONSTANT
	;
enumeration_constant:
	ENUMERATION_CONSTANT	
	;
declaration_specifiers_opt:
	declaration_specifiers { $$ = $1; }
	|
	;
declaration_list_opt:
	declaration_list { $$ = $1; }
	|
	;
init_declarator_list_opt:
	init_declarator_list { $$ = $1; }
	|
	;
identifier_opt:
	identifier { $$ = $1; }
	|
	;
specifier_qualifier_list_opt:
	specifier_qualifier_list { $$ = $1; }
	|
	;
declarator_opt:
	declarator { $$ = $1; }
	|
	;
pointer_opt:
	pointer { $$ = $1; }
	|
	;
constant_expression_opt:
	constant_expression { $$ = $1; }
	|
	;
identifier_list_opt:
	identifier_list { $$ = $1; }
	| {
		$$.list_pointer = createList();
		$$.is_vararg = FALSE;
	}
	;
type_qualifier_list_opt:
	type_qualifier_list { $$ = $1; }
	|
	;
abstract_declarator_opt:
	abstract_declarator { $$ = $1; }
	|
	;
direct_abstract_declarator_opt:
	direct_abstract_declarator { $$ = $1; }
	|
	;
parameter_type_list_opt:
	parameter_type_list { $$ = $1; }
	|
	;
expression_opt:
	expression { $$ = $1; }
	|
	;
statement_list_opt:
	statement_list { $$ = $1; }
	|
	;
argument_expression_list_opt:
	argument_expression_list { $$ = $1; }
	| {
		$$.list_pointer = createList();
		$$.is_vararg = FALSE;
	}
	;
definition_specifiers_opt:
	definition_specifiers { $$ = $1; }
	|
	;
%%
int main(int argc, char **argv) {
	int i;
	yystack code;
	char asmFile[255], intFile[255] , outFile[255] = "a.out", objFile[255];
	char *command;
	FILE *out;
	FILE *intcode;
	char outInt = 0;

	command = malloc(sizeof(char)*3000);

	if(argc >= 2) {
		for(i=1;i<argc;i++) {
			if(argv[i][0] == '-') {
				if(argv[i][1] == 'o') {
					strcpy(outFile, &argv[i][2]);
				}

			}
			else {
				strcpy(fileName, argv[i]);
				yyin = fopen(fileName,"r");
			}

		}
	}


	if(yyin == NULL) {
		printf("Err open file %s\n",fileName);
		return -1;
	}

	strcpy(asmFile, fileName);
	strtok(asmFile,".");
	strcat(asmFile, ".asm");
	out = fopen(asmFile,"w");

	strcpy(intFile, fileName);
	strtok(intFile,".");
	strcat(intFile, ".int");
	intcode = fopen(intFile,"w");

	strcpy(objFile, fileName);
	strtok(objFile,".");
	strcat(objFile, ".o");


	intCodeList = (void *)createList();
	refList = createList();
	initEnviron();
	yyparse();

	if(errCount == 0) {
		fprintf(out,"[bits 32]\n");
		fprintf(out,"section .text\n\n");
		
		printFunctions(out, intcode);
		fprintf(out,"\n\n");
		fprintf(intcode,"\n\n");
		printFunctionBody(out, intcode);
		fprintf(out,"\n\n");
		fprintf(intcode,"\n\n");
		printLiterals(out, intcode);

		fclose(out);
		fclose(intcode);

		sprintf(command, "nasm -felf -o%s %s",objFile, asmFile);
		system(command);
		sprintf(command, "ld -o%s /usr/lib/crt1.o /usr/lib/crti.o %s /usr/lib/crtn.o -lc --dynamic-linker /lib/ld-linux.so.2",outFile, objFile);
		system(command);
	}

	printf("%s: %d errors\n",argv[0], errCount);

	free(command);

	return 0;
}

void *concat_code(void *arg0, void *arg1, int *op1, int *op2) {
	int i,j;
	void *ret;
	yystack tmp_stack;

	ret = createList();
	for(i=0;i<getListSize(arg0);i++) {
		tmp_stack = getListElement(arg0,i);
		insertListEnd(ret, tmp_stack);
	}
	for(j=0;j<getListSize(arg1);j++) {
		tmp_stack = getListElement(arg1,j);
		tmp_stack.u_s.intcode.lineNo = i+j;
		if(!tmp_stack.u_s.intcode.not_modify_op1) {
			tmp_stack.u_s.intcode.op1 += i;
		}
		if(!tmp_stack.u_s.intcode.not_modify_op2) {
			tmp_stack.u_s.intcode.op2 += i;
		}
		insertListEnd(ret, tmp_stack);
	}

	deleteList(arg0);
	deleteList(arg1);

	*op1 = i-1;
	*op2 = i+j-1;
	return ret;
}

int insertRef(void *list, interCode code) {
	int i = 0;
	interCode tmpCode;
	for(i=0;i<getListSize(list);i++) {
		tmpCode = getListElement(list,i).u_s.intcode;
		if(tmpCode.command == code.command && tmpCode.op1 == code.op1) {
			return i;
		}
	}
	code.lineNo = i;
	tmp_stack.u_s.intcode = code;
	insertListEnd(list, tmp_stack);
	return i;
}

int insertCommand(yystack stack, int command, int op1, int op2, int nop1, int nop2) {
	interCode tmpCode;
	int line;

	line = getListSize(stack.list_pointer);
	tmpCode.lineNo = line;
	tmpCode.command = command;
	tmpCode.op1 = op1;
	tmpCode.op2 = op2;
	tmpCode.not_modify_op1 = nop1;
	tmpCode.not_modify_op2 = nop2;

	tmp_stack.u_s.intcode = tmpCode;
	insertListEnd(stack.list_pointer,tmp_stack);
}

int opType1(yystack *node, yystack arg0, int command) {
		if(arg0.node_type == TYPE_REFRENCE) {
			node->list_pointer = createList();
			tmp_stack.u_s.intcode.op1 = arg0.number;
			tmp_stack.u_s.intcode.op2 = -1;
			tmp_stack.u_s.intcode.lineNo = 0;
			tmp_stack.u_s.intcode.command = command;
			tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
			tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
			insertListEnd(node->list_pointer,tmp_stack);
		}
		else {
			node->list_pointer = concat_code(NULL, arg0.list_pointer,&op1,&op2);
			tmp_stack.u_s.intcode.op1 = op2;
			tmp_stack.u_s.intcode.op2 = -1;
			tmp_stack.u_s.intcode.lineNo = op2+1;
			tmp_stack.u_s.intcode.command = command;
			tmp_stack.u_s.intcode.not_modify_op1 = FALSE;
			tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
			insertListEnd(node->list_pointer,tmp_stack);
		}
		node->node_type = TYPE_ELSE;
		return 0;
}

int opType2(yystack *node, yystack arg0, yystack arg1, int command) {
	if(arg0.node_type != TYPE_REFRENCE && arg1.node_type != TYPE_REFRENCE) {
		node->list_pointer = concat_code(arg0.list_pointer, arg1.list_pointer,&op1,&op2);
		tmp_stack.u_s.intcode.op1 = op1;
		tmp_stack.u_s.intcode.op2 = op2;
		tmp_stack.u_s.intcode.lineNo = op2+1;
		tmp_stack.u_s.intcode.command = command;
		tmp_stack.u_s.intcode.not_modify_op1 = FALSE;
		tmp_stack.u_s.intcode.not_modify_op2 = FALSE;
		insertListEnd(node->list_pointer,tmp_stack);
	}
	else if(arg0.node_type == TYPE_REFRENCE && arg1.node_type != TYPE_REFRENCE) {
		node->list_pointer = concat_code(NULL, arg1.list_pointer,&op1,&op2);
		tmp_stack.u_s.intcode.op1 = arg0.number;
		tmp_stack.u_s.intcode.op2 = op2;
		tmp_stack.u_s.intcode.lineNo = op2+1;
		tmp_stack.u_s.intcode.command = command;
		tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
		tmp_stack.u_s.intcode.not_modify_op2 = FALSE;
		insertListEnd(node->list_pointer,tmp_stack);
	}
	else if(arg0.node_type != TYPE_REFRENCE && arg1.node_type == TYPE_REFRENCE) {
		node->list_pointer = concat_code(NULL, arg0.list_pointer,&op1,&op2);
		tmp_stack.u_s.intcode.op1 = op2;
		tmp_stack.u_s.intcode.op2 = arg1.number;
		tmp_stack.u_s.intcode.lineNo = op2+1;
		tmp_stack.u_s.intcode.command = command;
		tmp_stack.u_s.intcode.not_modify_op1 = FALSE;
		tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
		insertListEnd(node->list_pointer,tmp_stack);
	}
	else {
		node->list_pointer = createList();
		tmp_stack.u_s.intcode.op1 = arg0.number;
		tmp_stack.u_s.intcode.op2 = arg1.number;
		tmp_stack.u_s.intcode.lineNo = op2+1;
		tmp_stack.u_s.intcode.command = command;
		tmp_stack.u_s.intcode.not_modify_op1 = TRUE;
		tmp_stack.u_s.intcode.not_modify_op2 = TRUE;
		insertListEnd(node->list_pointer,tmp_stack);
	}
	node->node_type = TYPE_ELSE;
	return 0;
}

void yyerror(char *msg) {
	errMsg(msg);
}

int errMsg(char *msg) {
	errCount++;
	fprintf(stderr, "%s:%d: error: %s\n",fileName,lineNum, msg);
}
