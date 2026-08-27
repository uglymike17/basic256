#pragma once

#include <list>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>

#include <QString>
#include <QLocale>
#include <QColor>

#include "Error.h"
#include "Convert.h"
#include "DataElement.h"
#include "Settings.h"



class Stack
{
	public:
		Stack(Convert *);
		~Stack();

		Convert *convert;
		void pushDE(DataElement*);

		// The push/pop operations below are defined here rather than in
		// Stack.cpp because they are the innermost thing the interpreter
		// does - every operand of every expression goes through one - and
		// out of line they cost a call each with nothing left in registers
		// across it.

		void pushBool(bool i) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement(i?1LL:0LL);
		}

		void pushQString(QString string) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement(string);
		}

		void pushInt(int i) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement((qint64)i);
		}

		void pushLong(qint64 i) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement((qint64)i);
		}

		void pushRef(int i, int level) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer] = new DataElement();
			stackdata[stackpointer]->type = T_REF;
			stackdata[stackpointer]->intval = i;
			stackdata[stackpointer++]->level = level;
		}

		void pushDouble(double d) {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement(d);
		}

		void pushUnassigned() {
			if (stackpointer >= stacksize)  stackGrow();
			stackdata[stackpointer++] = new DataElement();
		}

		DataElement *popDE() {
			// pop an element - a POINTER to the data on the stack
			// WILL CHANGE ON NEXT PUSH!!!!

			// MUST delete THIS AFTER YOU ARE DONE WITH IT!!!!!!!!

			if (stackpointer==0) return popDEUnderflow();
			return stackdata[--stackpointer];
		}

		int peekType() {
			return peekType(0);
		}

		int peekType(int i) {
			if (stackpointer<=i) {
				e = ERROR_STACKUNDERFLOW;
				return T_UNASSIGNED;
			}
			return stackdata[stackpointer - i - 1]->type;
		}

		// Borrowed pointer to an element counted down from the top (0 is the
		// top). The stack keeps ownership - do NOT delete what this returns,
		// and it is only good until the next push or pop.
		DataElement *peekDE(int depth) {
			if (stackpointer<=depth) {
				e = ERROR_STACKUNDERFLOW;
				return NULL;
			}
			return stackdata[stackpointer - depth - 1];
		}

		// Discard the top element. The in-place binary operators work their
		// answer out in the element underneath and then drop the operand they
		// no longer need, which is how they avoid allocating a result.
		void dropTop() {
			if (stackpointer<=0) {
				e = ERROR_STACKUNDERFLOW;
				return;
			}
			stackpointer--;
			delete stackdata[stackpointer];
			stackdata[stackpointer] = NULL;
		}

		void swap();
		void swap2();
		void topto2();
		void dup();
		void dup2();
		int popInt();
		int popBool();
		QColor popQColor();
		qint64 popLong();
		double popDouble();
		double popMusicalNote();
		QString popQString();
		QString debug();
		int height();
		void drop(int);


		static int getError() {
			return getError(false);
		}

		static int getError(int clear) {
			int olde = e;
			if (clear) e = ERROR_NONE;
			return olde;
		}

	private:
		std::vector<DataElement*> stackdata;
		int stackpointer; //faster than unsigned int and is quite enough as size
		int stacksize;
		void stackGrow();
		DataElement *popDEUnderflow();	// cold path, kept out of line

		static int e;		// error number thrown - will be 0 if no error
};
