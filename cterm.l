%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "variable.h"

#define YYSTYPE yystack

#include "y.tab.h"

int lineNum = 1;
int definState = 0;
extern YYSTYPE yylval;

%}

%x	CCOMMENT
%x	PCOMMENT

typeName			void|char|short|int|long|float|double|signed|unsigned
storageClassName	auto|register|static|extern|typedef

%%
[ \t]+		{;}
\n			{lineNum++;}

"/*"			{BEGIN CCOMMENT;}
<CCOMMENT>\n	{lineNum++;}
<CCOMMENT>"*/"	{BEGIN INITIAL;}
<CCOMMENT>.*	{;}

"//"			{BEGIN PCOMMENT;}
<PCOMMENT>\n	{lineNum++;BEGIN INITIAL;}
<PCOMMENT>.*	{;}

case 		{return CASE;}
sizeof	 	{return SIZEOF;}
"||" 		{return LOGIC_OR;}
goto 		{return GOTO;}
do 			{return DO;}
while 		{return WHILE;}
if 			{return IF;}
for 		{return FOR;}
enum 		{return ENUM;}
else 		{return ELSE;}
"&&" 		{return LOGIC_AND;}
switch 		{return SWITCH;}
"!=" 		{return NEQUAL;}
break 		{return BREAK;}
">>" 		{return SHR;}
"<<" 		{return SHL;}
"..." 		{return ETC;}
struct		{return STRUCT;}
"<="		{return LTE;}
default		{return DEFAULT;}
"=="		{return EQUAL;}
"--"		{return MM;}
"++"		{return PP;}
continue	{return CONTINUE;}
union		{return UNION;}
">="		{return GTE;}
return		{return RETURN;}
"->"		{return ARROW;}

{storageClassName}/([^{;]|[\n])*\(([^;]|[\n])*\{	{ 
	if(!strcmp(yytext, "auto")) {
		yylval.number = STORAGE_AUTO;
	}
	else if(!strcmp(yytext, "register")) {
		yylval.number = STORAGE_REGISTER;
	}
	else if(!strcmp(yytext, "static")) {
		yylval.number = STORAGE_STATIC;
	}
	else if(!strcmp(yytext, "extern")) {
		yylval.number = STORAGE_EXTERN;
	}
	else if(!strcmp(yytext, "typedef")) {
		yylval.number = STORAGE_TYPEDEF;
	}

	return DEFINITION_STORAGE_CLASS;
}
{storageClassName} {
	if(!strcmp(yytext, "auto")) {
		yylval.number = STORAGE_AUTO;
	}
	else if(!strcmp(yytext, "register")) {
		yylval.number = STORAGE_REGISTER;
	}
	else if(!strcmp(yytext, "static")) {
		yylval.number = STORAGE_STATIC;
	}
	else if(!strcmp(yytext, "extern")) {
		yylval.number = STORAGE_EXTERN;
	}
	else if(!strcmp(yytext, "typedef")) {
		yylval.number = STORAGE_TYPEDEF;
	}

	return STORAGE_CLASS_SPECIFIER;
}
{typeName}/([^{;]|[\n])*\(([^;]|[\n])*\{ {
	if(!strcmp(yytext,"void")) {
		yylval.number = TYPE_VOID;
	}
	else if(!strcmp(yytext,"char")) {
		yylval.number = TYPE_CHAR;
	}
	else if(!strcmp(yytext,"short")) {
		yylval.number = TYPE_SHORT;
	}
	else if(!strcmp(yytext,"int")) {
		yylval.number = TYPE_INT;
	}
	else if(!strcmp(yytext,"float")) {
		yylval.number = TYPE_FLOAT;
	}
	else if(!strcmp(yytext,"double")) {
		yylval.number = TYPE_DOUBLE;
	}
	else if(!strcmp(yytext,"signed")) {
		yylval.number = TYPE_SIGNED;
	}
	else if(!strcmp(yytext,"unsigned")) {
		yylval.number = TYPE_UNSIGNED;
	}

	return DEFINITION_TYPE_SPECIFIER;
}
{typeName} {
	if(!strcmp(yytext,"void")) {
		yylval.number = TYPE_VOID;
	}
	else if(!strcmp(yytext,"char")) {
		yylval.number = TYPE_CHAR;
	}
	else if(!strcmp(yytext,"short")) {
		yylval.number = TYPE_SHORT;
	}
	else if(!strcmp(yytext,"int")) {
		yylval.number = TYPE_INT;
	}
	else if(!strcmp(yytext,"float")) {
		yylval.number = TYPE_FLOAT;
	}
	else if(!strcmp(yytext,"double")) {
		yylval.number = TYPE_DOUBLE;
	}
	else if(!strcmp(yytext,"signed")) {
		yylval.number = TYPE_SIGNED;
	}
	else if(!strcmp(yytext,"unsigned")) {
		yylval.number = TYPE_UNSIGNED;
	}

	return TYPE_SPECIFIER;
}
const|volatile {
	return TYPE_QUALIFIER;
}
"*="|"/="|"%="|"+="|"-="|"<<="|">>="|"&="|"^="|"|="|"=" {
	yylval.name = (char *)malloc(sizeof(char)*(strlen(yytext)+1));
	strcpy(yylval.name,yytext);
	return ASSIGNMENT_OPERATOR;
}
[\+\-\*\/\(\);{}\<\>,\%\[\]\&\~\|]	{yylval.number = *yytext;return *yytext;}
[0-9]+ {
	yylval.number = atoi(yytext);
	return INTEGER_CONSTANT;
}
\"[^\"\n]*\" {
	yylval.name = (char *)malloc(sizeof(char)*(strlen(yytext)+1));
	strcpy(yylval.name,yytext);
	return STRING;
}
[_a-zA-Z][_a-zA-Z0-9]* {
	yylval.name = (char *)malloc(sizeof(char)*(strlen(yytext)+1));
	strcpy(yylval.name,yytext);
	return IDENTIFIER;
}
\'.\'	{
	return CHARACTER_CONSTANT;
}
[0-9]*\.[0-9]+ {
	return FLOATING_CONSTANT;
}
%%
