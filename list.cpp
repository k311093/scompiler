#include <vector>
#include "common.h"

using namespace std;

extern "C" void* createList() {
	vector<yystack> *ret = new vector<yystack>;
	return (void *)ret;
}

extern "C" void deleteList(void *arg0) {
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	delete tmp;
}

extern "C" void insertList(void *arg0, int number, yystack arg1) {
	int i;
	vector<yystack>::iterator it;

	vector<yystack> *tmp = (vector<yystack> *)arg0;
	for(i=0;i<number;i++) {
		it++;	
	}
	tmp->insert(it,arg1);
}

extern "C" void insertListEnd(void *arg0, yystack arg1) {
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	tmp->push_back(arg1);
}

extern "C" int getListSize(void *arg0) {
	if(arg0 == NULL) {
		return 0;
	}
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	return tmp->size();
}

extern "C" yystack getListElement(void *arg0, int num) {
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	return (*tmp)[num];
}

extern "C" yystack *getListBack(void *arg0) {
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	return &tmp->back();
}

extern "C" yystack *getElementPointer(void *arg0, int num) {
	vector<yystack> *tmp = (vector<yystack> *)arg0;
	return &((*tmp)[num]);
}

