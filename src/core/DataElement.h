#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <limits>
#include <cstddef>
#include <new>

#include <QString>


#include "Error.h"
#include "BasicTypes.h"

// the DataElement.h and DataElements.cpp define a class that is
// used in stack and variable to store  single element of data and the
// definitions of the various types of values that may be stored

// we will pass a pointer to a DataElement to the functions of the
// Convert class for all output

#define MAXARRAYSIZE 1048576
class DataElement;

class DataElementArray
{
    public:
        int xdim;
        int ydim;
        // Elements live IN the vector, not behind a pointer each.  One
        // allocation holds the whole array, the elements are contiguous, and
        // a dimension no longer costs one trip to the allocator per element.
        std::vector<DataElement> data;
};


class DataElementMap
{
    public:
        std::map<std::string, DataElement*> data;
};

class DataElement
{
	public:
		// Members that can never be live at the same time share their storage,
		// which takes a DataElement from 72 bytes to 48 - and every value in
		// the system is one of these, on the stack, in a variable or in an
		// array.  (72 rather than the 48 the members suggest at a glance:
		// Qt6's QString is three words, not one.)
		// The type field says which member of each union is the live
		// one: T_INT/T_REF use intval, T_FLOAT uses floatval, T_ARRAY uses arr
		// and T_MAP uses map.  level is only meaningful for a T_REF, which
		// also uses intval, so it stays a member of its own - it costs nothing
		// because it sits in the padding that follows type.
		int type;	// type from BasicTypes.h
		int level;
		QString stringval;
		union {
			double floatval;
			qint64 intval;
		};
		union {
			DataElementArray *arr;
			DataElementMap *map;
		};


		DataElement();
		~DataElement();
		
		DataElement(QString);
		DataElement(double);
		DataElement(qint64);
		DataElement(int);
		DataElement(DataElement *);

		// DataElements are created and destroyed constantly - every stack push
		// and pop of the interpreter is one - so the raw blocks are recycled
		// through a free list instead of going back to the allocator each time.
		// Construction and destruction are untouched, only the memory is reused.
		static void* operator new(std::size_t);
		static void operator delete(void*) noexcept;

		QString debug();
		void copy(DataElement *);
		// Hand this element everything the source holds and leave the source
		// empty.  Unlike copy() it is O(1) whatever the element holds - an
		// array changes owner instead of being duplicated - which is what
		// taking a value off the stack wants.
		void stealFrom(DataElement *);
		// Exchange contents with another element, for the stack's SWAP family
		void swapWith(DataElement *);

		void clear();
		
		void arrayDim(const int, const int, const bool);
		DataElement* arrayGetData(const int, const int);
		void arraySetData(const int, const int, DataElement *);
		void arrayUnassign(const int, const int);
		int arrayRows();
		int arrayCols();

		void mapDim();
		DataElement* mapGetData(QString);
		void mapSetData(QString, DataElement *);
		void mapSetData(std::string, DataElement *);
		void mapUnassign(QString);
		int mapLength();
		bool mapKey(QString);
		
		static int getError();
		static int getError(int);
		static int getType(DataElement*);

	private:
		static int e;		// error number thrown - will be 0 if no error
		void init();
};



