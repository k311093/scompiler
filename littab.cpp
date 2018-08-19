#include <vector>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>

using namespace std;

vector<int> numLitList;
vector<char> charLitList;
vector<float> floatLitList;
vector<char *> strLiterals;

extern "C" int numLiteral(int num) {
	int i=0;
	for(i=0;i<numLitList.size();i++) {
		if(numLitList[i] == num) {
			return i;
		}
	}
	numLitList.push_back(num);
	return numLitList.size() - 1;
}

extern "C" void printLiterals(FILE *fp, FILE *intcode) {
	int i,j;

	for(i=0;i<numLitList.size();i++) {
		fprintf(fp,"_num_lit%d:\n\t dd %d\n",i,numLitList[i]);
		fprintf(intcode, "Number Literal %d = %d\n",i, numLitList[i]);
	}
	fprintf(intcode, "\n");
	for(i=0;i<strLiterals.size();i++) {
		fprintf(fp,"_str_lit%d:\n\t db ",i);
		for(j=0;j<strlen(strLiterals[i]);j++) {
			fprintf(fp, "0x%02x, ",(unsigned char)strLiterals[i][j]); 
		}
		fprintf(fp, "0x%02x\n",strLiterals[i][j]); 
		fprintf(intcode, "String Literal %d = \"%s\"\n", i, strLiterals[i]);
	}
}

extern "C" int strLiteral(char *text) {
	int i;
	int count;
	char *str;


	str = (char *)malloc(sizeof(char)*strlen(text)-1);

	count = 0;

	for(i=1;i<strlen(text)-1;i++) {
		if(text[i] != '\\')
			str[count++] = text[i];
		else {
			i++;
			switch(text[i]) {
				case 'n':
					str[count++] = '\n';
					break;
				case 't':
					str[count++] = '\t';
					break;
				case '\\':
					str[count++] = '\\';
					break;
			}
		}
	}

	str[count] = 0;

	for(i=0;i<strLiterals.size();i++) {
		if(!strcmp(strLiterals[i], str)) {
			free(str);
			return i;
		}
	}

	strLiterals.push_back(str);
	return strLiterals.size() - 1;
}
