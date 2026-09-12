#include "Stack.h"
#include "DataElement.h"
#include <string>

int Stack::e = ERROR_NONE;

Stack::Stack(Convert *c) {
	convert = c;
	stackpointer = 0;	// height of stack
	stacksize = 0;       //max size of stack, so the depth never has to be asked for
	stackGrow();
}

Stack::~Stack() {
	for (size_t i = 0; i < chunks.size(); i++) {
		delete [] chunks[i];
	}
	chunks.clear();
}

void Stack::stackGrow() {
	// one more chunk of ready-built slots.  The slots are constructed here,
	// once, and then reused by every push that lands on them; the chunks
	// already handed out are never moved or freed, so an element borrowed
	// from one stays where it is.
	chunks.push_back(new DataElement[CHUNKSLOTS]);
	stacksize += CHUNKSLOTS;
}

QString Stack::debug() {
	// return a string representing the stack
	QString s("");
	for (int i=0; i<stackpointer; i++) {
		s += at(i)->debug() +  " ";
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
	// push to stack a copy of the dataelement

	// IF YOU CREATED A DE TO PUSH - BE SURE TO DELETE
	// AFTER pushDE

	DataElement *s = nextSlot();
	if (source) {
		s->copy(source);
	} else {
		s->clear();
	}
}

//
// Raw Pop Operations

DataElement *Stack::borrowUnderflow() {
	// the borrowed pops hand back an element the stack owns, so underflow
	// cannot answer with something the caller would have to free
	e = ERROR_STACKUNDERFLOW;
	underflowSlot.clear();
	underflowSlot.type = T_INT;
	underflowSlot.intval = 0l;
	return &underflowSlot;
}

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
	bool b = convert->getBool(at(--stackpointer));
	at(stackpointer)->clear();
	return b;
}

int Stack::popInt() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	int i = convert->getInt(at(--stackpointer));
	at(stackpointer)->clear();
	return i;
}

qint64 Stack::popLong() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0;
	}
	qint64 l = convert->getLong(at(--stackpointer));
	at(stackpointer)->clear();
	return l;
}

double Stack::popDouble() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0.0;
	}
	double f = convert->getFloat(at(--stackpointer));
	at(stackpointer)->clear();
	return f;
}

double Stack::popMusicalNote() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return 0.0;
	}
	double f = convert->getMusicalNote(at(--stackpointer));
	at(stackpointer)->clear();
	return f;
}

QString Stack::popQString() {
	if (stackpointer==0) {
		e = ERROR_STACKUNDERFLOW;
		return QString("");
	}
	QString s = convert->getString(at(--stackpointer));
	at(stackpointer)->clear();
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

	if (stackpointer<4) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	at(stackpointer-3)->swapWith(at(stackpointer-1));
	at(stackpointer-4)->swapWith(at(stackpointer-2));
}

void Stack::swap() {
	// swap top two elements
	// if top of stack is A,B,C,D make it B,A,C,D

	if (stackpointer<2) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	at(stackpointer-2)->swapWith(at(stackpointer-1));
}

void
Stack::topto2() {
	// move the top of the stack under the next two
	// 0, 1, 2, 3...  becomes 1, 2, 0, 3...

	if (stackpointer<3) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	
	// 0,1,2 -> 1,2,0 by two exchanges
	at(stackpointer-1)->swapWith(at(stackpointer-2));
	at(stackpointer-2)->swapWith(at(stackpointer-3));
}

void Stack::dup() {
	// make copy of top
	// if top of stack is A,B,C,D make it A,A,B,C,D
	if (stackpointer<1) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	pushDE(at(stackpointer-1));
}

void Stack::dup2() {
	// make copy of top two
	// if top of stack is A,B,C,D make it A,B,A,B,C,D
	if (stackpointer<2) {
		e = ERROR_STACKUNDERFLOW;
		return;
	}
	pushDE(at(stackpointer-2));
	pushDE(at(stackpointer-2));
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
		at(--stackpointer)->clear();
	}
}
