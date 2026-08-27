#include "Stack.h"
#include "DataElement.h"
#include <string>

int Stack::e = ERROR_NONE;

Stack::Stack(Convert *c) {
	convert = c;
	stackpointer = 0;	// height of stack
	stacksize = 0;       //max size of stack to avoid calling stackdata.size()
	stackGrow();
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
	// add 10 empty slots to the size of the stack
	// the slots are left NULL - every push allocates the element it stores, so
	// filling them in advance saved nothing and leaked each pre-made element
	// as soon as a push overwrote its pointer
	stackdata.resize(stacksize+10, NULL);
	stacksize=stackdata.size();
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
	
	// IF YOU CREATED A DE TO PUSH - BE SURE TO DELETE
	// AFTER pushDE
	
	stackdata[stackpointer] = new DataElement();
	if (source) {
		stackdata[stackpointer]->copy(source);
	}
	stackpointer++;
}

//
// Raw Pop Operations

DataElement *Stack::popDEUnderflow() {
	// the cold half of popDE() - kept out of line so the inline fast path in
	// Stack.h stays small enough to be worth inlining
	e = ERROR_STACKUNDERFLOW;
	// return a fake element instead of NULL
	// to handle a potential error in Interpreter
	DataElement *de = new DataElement();
	de->type = T_INT;
	de->intval = 0l;
	return de;
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

qint64 Stack::popLong() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	qint64 l = convert->getLong(stackdata[--stackpointer]);
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

QColor Stack::popQColor() {
	if (peekType() == T_STRING) {
		QString s = popQString();
		if (QString::compare(s, "CLEAR", Qt::CaseInsensitive)) {
			return QColor(s);
		} else {
			return Qt::transparent;
		}
	} else {
		return QColor::fromRgba((QRgb) popInt());
	}
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
	// the dropped elements are deleted - nobody else holds a pointer to them
	while (n-- > 0) {
		if (stackpointer<=0) {
			stackpointer=0;
			e = ERROR_STACKUNDERFLOW;
			return;
		}
		stackpointer--;
		delete stackdata[stackpointer];
		stackdata[stackpointer] = NULL;
	}
}
