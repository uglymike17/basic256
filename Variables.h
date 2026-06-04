// implement variables using a c++ map of the variable number and a pointer to the variable structure
// this will allow for a dynamically allocated number of variables
// 2010-12-13 j.m.reneau

#pragma once

#include "Error.h"
#include "DataElement.h"

#define MAX_RECURSE_LEVELS	1048576


class Variable
{
	public:
		Variable();
		~Variable();

		DataElement *data;
};


class Variables: public QObject
{
	Q_OBJECT;
	public:
		Variables(int);
		~Variables();
		//
		QString debug();
		void increaserecurse();
		void decreaserecurse();
		int getrecurse();
		//
		int type(int);
		//
		Variable* get(int, int);
		Variable* get(int);
		Variable* getAt(int, int);
		Variable* getAt(int);
		DataElement *getData(int);

		void setData(int, DataElement *);
		void setData(int, long);
		void setData(int, double);
		void setData(int, QString);
		void setData(int, std::string);
		void unassign(int);
		//
		void makeglobal(int);

		static int getError() {
			return getError(false);
		}

		static int getError(int clear) {
			int olde = e;
			if (clear) e = ERROR_NONE;
			return olde;
		}

	private:
		int real_varnum;		// set by get and getAt for the actual variable number and level returned (deref/global)
		int real_level;
		int numsyms;		// size of the symbol table
		int recurselevel;
		std::vector<Variable**> varmap;
		bool *isglobal;
		void clearvariable(Variable *);
		static int e;		// error number thrown - will be 0 if no error
};
