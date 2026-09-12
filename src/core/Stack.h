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
		//
		// A slot IS a DataElement now, not a pointer to one.  The slots are
		// built once, when their chunk is made, and then reused for the life
		// of the run: a push assigns into the slot it lands on instead of
		// asking the allocator for an element and a pop leaves the slot where
		// it is.  So the whole allocate-construct-destroy-free cycle that used
		// to happen for every operand of every expression is gone.

		void pushBool(bool i) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_INT;
			s->intval = i?1LL:0LL;
		}

		void pushQString(QString string) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_STRING;
			s->stringval = string;
		}

		void pushInt(int i) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_INT;
			s->intval = (qint64)i;
		}

		void pushLong(qint64 i) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_INT;
			s->intval = i;
		}

		void pushRef(int i, int level) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_REF;
			s->intval = i;
			s->level = level;
		}

		void pushDouble(double d) {
			DataElement *s = nextSlot();
			s->clear();
			s->type = T_FLOAT;
			s->floatval = d;
		}

		void pushUnassigned() {
			nextSlot()->clear();
		}

		// Borrowed pop: the element stays the stack's, and stays valid only
		// until the next push.  Do NOT delete what this returns.  This is what
		// the hot opcodes use; popDE() below is the owning version the rest of
		// the interpreter still expects.
		DataElement *popDEborrow() {
			if (stackpointer==0) return borrowUnderflow();
			return at(--stackpointer);
		}

		DataElement *popDE() {
			// pop an element - a POINTER to the data on the stack
			// WILL CHANGE ON NEXT PUSH!!!!

			// MUST delete THIS AFTER YOU ARE DONE WITH IT!!!!!!!!

			// The slot cannot be handed out - it belongs to the stack and is
			// about to be reused - so its contents move into an element of
			// the caller's own.  Moving, not copying: an array on the stack
			// would otherwise be deep copied a second time.
			if (stackpointer==0) return popDEUnderflow();
			DataElement *s = at(--stackpointer);
			DataElement *r = new DataElement();
			r->stealFrom(s);
			return r;
		}

		int peekType() {
			return peekType(0);
		}

		int peekType(int i) {
			if (stackpointer<=i) {
				e = ERROR_STACKUNDERFLOW;
				return T_UNASSIGNED;
			}
			return at(stackpointer - i - 1)->type;
		}

		// Borrowed pointer to an element counted down from the top (0 is the
		// top). The stack keeps ownership - do NOT delete what this returns,
		// and it is only good until the next push or pop.
		DataElement *peekDE(int depth) {
			if (stackpointer<=depth) {
				e = ERROR_STACKUNDERFLOW;
				return NULL;
			}
			return at(stackpointer - depth - 1);
		}

		// Discard the top element. The in-place binary operators work their
		// answer out in the element underneath and then drop the operand they
		// no longer need, which is how they avoid allocating a result.
		void dropTop() {
			if (stackpointer<=0) {
				e = ERROR_STACKUNDERFLOW;
				return;
			}
			// the slot stays where it is - clearing it is enough to release
			// anything it was holding
			at(--stackpointer)->clear();
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
		// The slots live in chunks that are never moved or freed while the
		// program runs, so a borrowed element cannot be invalidated by the
		// stack growing under it - which a single reallocating buffer would
		// do, and which the old array of pointers never had to worry about.
		enum { CHUNKBITS = 8, CHUNKSLOTS = 1 << CHUNKBITS, CHUNKMASK = CHUNKSLOTS - 1 };
		std::vector<DataElement*> chunks;
		int stackpointer; //faster than unsigned int and is quite enough as size
		int stacksize;

		DataElement *at(int i) {
			return chunks[i >> CHUNKBITS] + (i & CHUNKMASK);
		}

		DataElement *nextSlot() {
			if (stackpointer >= stacksize) stackGrow();
			return at(stackpointer++);
		}

		void stackGrow();
		DataElement *popDEUnderflow();	// cold path, kept out of line
		DataElement *borrowUnderflow();	// the same for the borrowed pops
		DataElement underflowSlot;		// what a borrowed pop answers underflow with

		static int e;		// error number thrown - will be 0 if no error
};
