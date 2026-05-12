#include "Stack.h"
#include "DataElement.h"
#include <string>

int Stack::e = ERROR_NONE;

Stack::Stack(Convert *c, QLocale *applocale) {
	convert = c;
	stackpointer = 0;	// height of stack
	stacksize = 0;       //max size of stack to avoid calling stackdata.size()
	stackGrow();
	locale = applocale;

}

Stack::~Stack() {
	for(int i = 0; i< stackpointer; i++) {
		if (stackdata[i]) {
			delete(stackdata[i]);
			stackdata[i] = NULL;
		}
	}
	stackdata.clear();
}

void Stack::stackGrow() {
	// add 10 elements to the size of the stack
	int i = stacksize;
	stackdata.resize(stacksize+10);
	stacksize=stackdata.size();
	while(i< stacksize) {
		stackdata[i] = new DataElement();
		i++;
	}
}

QString Stack::debug() {
	// return a string representing the stack
	QString s("");
	for (int i=0; i<stackpointer; i++) {
		s += stackdata[i]->debug() +  " ";
	}
	return s;
}

int Stack::height() {
	// return the height of the stack in elements
	// magic of pointer math returns number of elements
	return stackpointer;
}

//
// RAW Push Operations
//

void Stack::pushDE(DataElement *source) {
	if (stackpointer >= stacksize)  stackGrow();
	// push to stack a copy of he dataelement
	stackdata[stackpointer] = new DataElement();
	if (source) {
		stackdata[stackpointer]->copy(source);
	} else {
		stackdata[stackpointer]->type = T_UNASSIGNED;
	}
	stackpointer++;
}

void Stack::pushLong(long i) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer++] = new DataElement(i);
}

void Stack::pushRef(int i, int level) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer] = new DataElement();
	stackdata[stackpointer]->type = T_REF;
	stackdata[stackpointer]->intval = i;
	stackdata[stackpointer++]->level = level;
}

void Stack::pushDouble(double d) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer++] = new DataElement(d);
}

void Stack::pushQString(QString string) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer++] = new DataElement(string);
}

void Stack::pushInt(int i) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer++] = new DataElement((long)i);
}

void Stack::pushBool(bool i) {
	if (stackpointer >= stacksize)  stackGrow();
	stackdata[stackpointer++] = new DataElement(i?1L:0L);
}


//
// Pushes derived from RAW pushes
//

void Stack::pushVariant(QString string, int type) {
	// try to convert a string to an int or float and push that type
	// if unable then push a string
	switch (type) {
		case T_UNASSIGNED:
			{
				bool ok;
				long i;
				i = string.toLong(&ok);
				if (ok) {
					pushLong(i);
				} else {
					double d;
					d = locale->toDouble(string,&ok);
					if (ok) {
						pushDouble(d);
					} else {
						// not an integer or double - push string
						pushQString(string);
					}
				}
			}
			break;
		case T_INT:
			{
				bool ok;
				long i=0;
				i = string.toLong(&ok);
				if (!ok) {
					e = ERROR_NUMBERCONV;
				}
				pushLong(i);
			}
			break;
		case T_FLOAT:
			{
				bool ok;
				double d=0.0;
				d = locale->toDouble(string,&ok);
				if (!ok) {
					e = ERROR_NUMBERCONV;
				}
				pushDouble(d);
			}
			break;
		case T_STRING:
			{
				pushQString(string);
			}
			break;
	}
}
	


//
// Peek Operations - look but dont touch

int Stack::peekType() {
	return peekType(0);
}

int Stack::peekType(int i) {
	if (stackpointer<=i) {
		e = ERROR_STACKUNDERFLOW;
		return T_UNASSIGNED;
	}
	return stackdata[stackpointer - i - 1]->type;
}

//
// Raw Pop Operations

DataElement *Stack::popDE() {
	// pop an element - a POINTER to the data on the stack
	// WILL CHANGE ON NEXT PUSH!!!!
	
	// MUST delete THIS AFTER YOU ARE DOME WITH IT!!!!!!!!
	
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		// return a fake element instead of NULL
		// to handle a potential error in Interpreter
		DataElement *de = new DataElement();
		de->type = T_INT;
		de->intval = 0l;
		return de;
	}
	stackpointer--;
	return stackdata[stackpointer];
}

int Stack::popBool() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	bool b = convert->getBool(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return b;
}

int Stack::popInt() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	int i = convert->getInt(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return i;
}

long Stack::popLong() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	long l = convert->getLong(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return l;
}

double Stack::popDouble() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0.0;
	}
	double f = convert->getFloat(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return f;
}

double Stack::popMusicalNote() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0.0;
	}
	double f = convert->getMusicalNote(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return f;
}

QString Stack::popQString() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return QString("");
	}
	QString s = convert->getString(stackdata[--stackpointer]);
	delete stackdata[stackpointer];
	return s;
}

//
// SWAP and DUP opeations to the stack

void Stack::swap2() {
	// swap top two pairs of elements
	// if top of stack is A,B,C,D make it C,D,A,B
	DataElement *t;

	if (stackpointer<4) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	t = stackdata[stackpointer-3];
	stackdata[stackpointer-3] = stackdata[stackpointer-1];
	stackdata[stackpointer-1] = t;

	t = stackdata[stackpointer-4];
	stackdata[stackpointer-4] = stackdata[stackpointer-2];
	stackdata[stackpointer-2] = t;
}

void Stack::swap() {
	// swap top two elements
	// if top of stack is A,B,C,D make it B,A,C,D
	DataElement *t;

	if (stackpointer<2) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	t = stackdata[stackpointer-2];
	stackdata[stackpointer-2] = stackdata[stackpointer-1];
	stackdata[stackpointer-1] = t;
}

void
Stack::topto2() {
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
	DataElement *t;

	if (stackpointer<3) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	t = stackdata[stackpointer-1];
	stackdata[stackpointer-1] = stackdata[stackpointer-2];
	stackdata[stackpointer-2] = stackdata[stackpointer-3];
	stackdata[stackpointer-3] = t;
}

void Stack::dup() {
	// make copy of top
	// if top of stack is A,B,C,D make it A,A,B,C,D
	if (stackpointer<1) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	pushDE(stackdata[stackpointer-1]);
}

void Stack::dup2() {
	// make copy of top two
	// if top of stack is A,B,C,D make it A,B,A,B,C,D
	if (stackpointer<2) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	pushDE(stackdata[stackpointer-2]);
	pushDE(stackdata[stackpointer-2]);
}

void Stack::drop(int n){
	//quick drop a number of elements from stack
	//usefull to clear the stack when an array from stack is not needed anymore
	//in case that error is catched and we want to pass over that (ONERROR or TRY/CATCH)
	stackpointer-=n;
	if (stackpointer<0) {
		stackpointer=0;
		e = ERROR_STACKUNDERFLOW;
	}
}
