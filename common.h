#pragma once
#include "variable.h"
#include "declspec.h"
#include "intercode.h"

#define TRUE 1
#define FALSE 0

enum {TYPE_VOID, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_FLOAT, TYPE_DOUBLE,
	  TYPE_SIGNED, TYPE_UNSIGNED};
enum {STORAGE_AUTO, STORAGE_REGISTER, STORAGE_STATIC, STORAGE_EXTERN,
	  STORAGE_TYPEDEF};
enum {QUALIFIER_CONST, QUALIFIER_VOLATILE};
enum {TYPE_ELSE, TYPE_LABELED,TYPE_EXPRESSION,TYPE_COMPOUND,TYPE_SELECTION,
	  TYPE_ITERATION,TYPE_JUMP,TYPE_REFRENCE,TYPE_VARDECL,
	  TYPE_FUNCDECL};
enum {OPERATOR_NONE = -1, OPERATOR_BREAK = -2, OPERATOR_CONTINUE = -3};

#define SET_BIT(x,n)	((x) |= 1<<(n))
#define IS_SET(x,n)		((x) & 1<<(n))
#define CLEAR_BIT(x,n)	((x) &= ~(1<<(n)))

typedef struct {
	int number;
	int node_type;
	char *name;
	void *list_pointer;
	char is_vararg;
	union {
		interCode intcode;
		declaration_specifier declspec;
	} u_s;
} yystack;

