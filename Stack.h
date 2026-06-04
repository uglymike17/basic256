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
		void pushBool(bool);
		void pushQString(QString);
		void pushInt(int);
		void pushLong(long);
		void pushRef(int, int);
		void pushDouble(double);
		void pushUnassigned();
		void swap();
		void swap2();
		void topto2();
		void dup();
		void dup2();
		int peekType();
		int peekType(int);
		DataElement *popDE();
		int popInt();
		int popBool();
		QColor popQColor();
		long popLong();
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
		
		static int e;		// error number thrown - will be 0 if no error
};
