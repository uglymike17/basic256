#include "Variables.h"
#include <QDebug>

// manage storage of variables

// variables are limited to types (defined in Types.h):

int Variables::e = ERROR_NONE;


// T_REF is a variable number in the specified recursion level

// ************************************************************
// * the Variable object - defines a single value or an array *
// ************************************************************

Variable::Variable() {
	data = new DataElement();
}

Variable::~Variable() {
	delete(data);
}


// ************************************************************************
// * Variables contain the individual variable(s) for the recursion level *
// ************************************************************************

Variables::Variables(int n) :  numsyms(n){
	// n is the size of the symbol table
    recurselevel = 0;
	maxrecurselevel = -1;
	allocateRecurseLevel();
	isglobal = new bool[numsyms];
	int i=numsyms;
	while(i-- > 0) {
		isglobal[i]=false;
	}
}

Variables::~Variables() {
	// free variables and all storage
	do {
		// delete a recurse level being sure to delete
		// the variables in the runlevel before removing it
		int i=numsyms;
		while(i-- > 0) {
			delete(varmap[recurselevel][i]);
		}
		delete[] varmap[recurselevel];
		recurselevel--;
	} while (recurselevel>=0);
	varmap.clear();
	delete[] isglobal;
}

QString Variables::debug() {
	// return a string representing the variables
	QString s("");
	if(!varmap.empty()) {
		for(int i=0;i<=recurselevel;i++) {
			s += QStringLiteral("recurse Level ") + QString::number(i) + "\n";
			for(int j=0; j < numsyms; j++) {
				s += QStringLiteral(" varnum ") + QString::number(j) + QStringLiteral(" ") ;
				s += varmap[i][j]->data->debug();
				s += "\n";
            }
            s += "\n";
        }
    }
    return s;
}

void Variables::allocateRecurseLevel() {
    // initialize a new recurse level
    // create a new Variable for each symbol at this recurse level
    // wasteful for the labels and jump points but vector is so fast
    // to access an element it does not matter
    if(maxrecurselevel<recurselevel){
        varmap.resize(recurselevel+1);
        varmap[recurselevel] = new Variable*[numsyms];
        int i=numsyms;
        while(i-- > 0) {
            varmap[recurselevel][i] = NULL;
        }
        
        maxrecurselevel=recurselevel;
    }
}

void Variables::clearRecurseLevel() {
	// clear a recurse level
	//it can be reused faster than create/delete all variables
	int i=numsyms;
	while(i-- > 0) {
		delete varmap[recurselevel][i];
		varmap[recurselevel][i] = NULL;
    }
}

void Variables::increaserecurse() {
    recurselevel++;
    if (recurselevel>=MAX_RECURSE_LEVELS) {
        recurselevel--;
        e = ERROR_MAXRECURSE;
    } else {
		allocateRecurseLevel();
	}
}

int Variables::getrecurse() {
    return recurselevel;
}

void Variables::decreaserecurse() {
    if (recurselevel>0) {
        // clear all variables in the current recurse level before we return
        // to the previous level
        clearRecurseLevel();
        // return back to prev variable context
        recurselevel--;
    }
}


Variable* Variables::get(int varnum, int level) {
	// get v from map or follow REF to recurse level if needed
	Variable *v;
	//printf("variables::get - varnum %d level %d\n", varnum, level);
	v = getAt(varnum, level);
	if(v->data->type==T_REF) {
		// reference to other level - recurse down the rabitty hole
		return get(v->data->intval, v->data->level);
	} else {
		return(v);
	}
}

Variable* Variables::get(int varnum) {
    return get(varnum, recurselevel);
}

Variable* Variables::getAt(int varnum, int level) {
	// get variable from map without follow REF
	// need for variable window too
	Variable *v;
	if(isglobal[varnum]) level = 0;
	v = varmap[level][varnum];
	if (not v) {
		v = new Variable();
		varmap[level][varnum] = v;
	}
	return v;
}

Variable* Variables::getAt(int varnum) {
	// get variable without recurse from this level
	return getAt(varnum, recurselevel);
}

DataElement* Variables::getData(int varnum) {
	// get data from variable - return varnum's data
	Variable *v = get(varnum);
	return v->data;
}


void Variables::setData(int varnum, DataElement* d) {
    // recieves a DataElement pointed pulled from the stack
    // e is a pointer in the stack vector AND MUST NOT BE DELETED
	Variable *v;
	if (d->type==T_REF) {
		// if we are assigning a "REF" dont recurse down the previous ref is there is one
		// just assign it at the current run level.
		v = getAt(varnum);
	} else {
		v = get(varnum, recurselevel);
	}
	v->data->copy(d);
}

void Variables::setData(int varnum, long l) {
    Variable *v = get(varnum);
    v->data->type = T_INT;
    v->data->intval = l;
}

void Variables::setData(int varnum, double f) {
    Variable *v = get(varnum);
    v->data->type = T_FLOAT;
    v->data->floatval = f;
}

void Variables::setData(int varnum, QString s) {
    Variable *v = get(varnum);
    v->data->type = T_STRING;
    v->data->stringval = s;
}

void Variables::unassign(int varnum) {
    //unassign true variable, not the reference (unlink)
    Variable *v = varmap[(recurselevel==0||isglobal[varnum])?0:recurselevel][varnum];
    v->data->clear();
}

void Variables::makeglobal(int varnum) {
    // make a variable global - if there is a value in a run level greater than 0 then
    // that value will be lost to the program
    isglobal[varnum] = true;
}
