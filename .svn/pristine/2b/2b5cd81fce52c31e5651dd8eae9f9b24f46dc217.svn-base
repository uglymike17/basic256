#include "DataElement.h"
#include "Settings.h"

#include <string>

int DataElement::e = ERROR_NONE;

void DataElement::init() {
	// initialize dataelement common stuff
	e=0;
	arr = NULL;
	map = NULL;
}

DataElement::DataElement() {
	// create an empty dataelement
	init();
	type = T_UNASSIGNED;
}

DataElement::DataElement(QString s) {
	init();
	type = T_STRING;
	stringval = s;
}

DataElement::DataElement(int i) {
	init();
    type = T_INT;
    intval = i;
}

DataElement::DataElement(long l) {
	init();
	type = T_INT;
	intval = l;
}

DataElement::DataElement(double d) {
	init();
	type = T_FLOAT;
	floatval = d;
}

DataElement::~DataElement() {
	stringval.clear();
	if (arr){
		delete(arr);
		arr=NULL;
	}
}




void DataElement::copy(DataElement *source) {
	// fill an existing from as a copy of another
	// it is as fast as it can be
	// unused values of strings can be found as garbage but this is the cost of speed
	// source->stringval.clear() is too expensive to be used for each assignment/copy
	switch (source->type){
		case T_UNASSIGNED:
			break;
		case T_FLOAT:
			floatval = source->floatval;
			break;
		case T_STRING:
			stringval = source->stringval;
			break;
		case T_ARRAY:
			{
				// copy array elements from one old array to a new one
				if (!arr)
					arr = new DataElementArray;
				int i = source->arr->xdim * source->arr->ydim;
				arr->xdim = source->arr->xdim;
				arr->ydim = source->arr->ydim;
				arr->data.resize(i);
				while(i-- > 0) {
					arr->data[i].copy(&source->arr->data[i]);
				}
			}
			break;
		case T_MAP:
			{
#ifdef DEBUG
fprintf(stderr,"de copy map source len %d\n",source->map->data.size());
#endif
				if (type==T_MAP) {
					map->data.clear();
				} else {
					map = new DataElementMap;
				}
				map->data.insert(source->map->data.begin(), source->map->data.end());
			}
			break;
		case T_REF:
			intval = source->intval;
			level = source->level;
			break;
		case T_INT:
			intval = source->intval;
	}
	type = source->type;
}

void DataElement::clear() {
	type = T_UNASSIGNED;
	intval = 0;
	stringval.clear();
	level = 0;
	if (arr){
		delete(arr);
		arr = NULL;
	}
	if (map) {
		delete(map);
		map = NULL;
	}
}


QString DataElement::debug() {
	// return a string representing the DataElement contents
	if(type==T_INT) return("int(" + QString::number(intval) + ") ");
	if(type==T_REF) return("varref(" + QString::number(intval) + ", level: " + QString::number(level) + ") ");
	if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
	if(type==T_STRING) return("string(" + stringval + ") ");
	if(type==T_ARRAY) return("array(" + QString::number(intval) + ")");
	if(type==T_MAP) return("MAP()");
	if(type==T_UNASSIGNED) return("unused ");
	return("badtype ");
}

void DataElement::arrayDim(const int xdim, const int ydim, const bool redim) {
	const int size = xdim * ydim;
	
	if (size <= MAXARRAYSIZE) {
		if (size >= 1) {
			if (type != T_ARRAY || !redim) {
				// if array data is dim or redim without a dim then create a new one (clear the old)
				if (!arr){
					arr = new DataElementArray;
				}else{
					arr->data.clear();
				}
				arr->data.resize(size);
				//int i = size;
				//while(i-- > 0) {
				//	arr->data[i].intval = 0;
				//}
				type = T_ARRAY;
			}else{
				// redim - resize the vector
				//const int oldsize = arr->xdim * arr->ydim;
				arr->data.resize(size);
				//for(int i=oldsize;i<size;i++) {
				//	arr->data[i].intval = 0;
				//}
			}

			arr->xdim = xdim;
			arr->ydim = ydim;
		} else {
			e = ERROR_ARRAYSIZESMALL;
		}
	} else {
		e = ERROR_ARRAYSIZELARGE;
	}
}

int DataElement::arrayRows() {
	// return number of columns of array as if it was a two dimensional array - 0 = not an array
	if (type == T_ARRAY) {
		return(arr->xdim);
	} else if (type==T_UNASSIGNED){
		e = ERROR_VARNOTASSIGNED;
	} else {
		e = ERROR_NOTARRAY;
	}
	return(0);
}

int DataElement::arrayCols() {
	// return number of rows of array as if it was a two dimensional array - 0 = not an array
	if (type == T_ARRAY) {
		return(arr->ydim);
	} else if (type==T_UNASSIGNED){
		e = ERROR_VARNOTASSIGNED;
	} else {
		e = ERROR_NOTARRAY;
	}
	return(0);
}

DataElement* DataElement::arrayGetData(const int x, const int y) {
	// get data from array elements from map (using x, y)
	// if there is an error return an unassigned value
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			DataElement *d = &arr->data[i];
			return d;
		} else {
			e = ERROR_ARRAYINDEX;
		}
	} else {
		e = ERROR_NOTARRAY;
	}
	return this;
}

void DataElement::arraySetData(const int x, const int y, DataElement *d) {
	// DataElement's data is copied and it should be deleted
	// by whomever created it
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			arr->data[i].copy(d);
		} else {
			e = ERROR_ARRAYINDEX;
		}
	} else {
		e = ERROR_NOTARRAY;
	}
}

void DataElement::arrayUnassign(const int x, const int y) {
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			arr->data[i].type = T_UNASSIGNED;
		} else {
			e = ERROR_ARRAYINDEX;
		}
	} else {
		e = ERROR_NOTARRAY;
	}
}



void DataElement::mapDim(){
	if (type==T_MAP) {
		map->data.clear();
	} else {
		clear();
		map = new DataElementMap();
		type=T_MAP;
	}
}

DataElement* DataElement::mapGetData(QString key){
	DataElement *d;
	if (type==T_MAP) {
		if (map->data.count(key)) {
			d = &map->data[key];
		} else {
			e = ERROR_MAPKEY;
			d = new DataElement();
		}
	} else {
		e = ERROR_NOTMAP;
		d = new DataElement();
	}
	return d;
}

void DataElement::mapSetData(QString key, DataElement *d){
	if (type==T_MAP) {
		map->data[key] = *d;
	} else {
		e = ERROR_NOTMAP;
	}
}

void DataElement::mapUnassign(QString key){
	if (type==T_MAP) {
		if (map->data.count(key)) {
			map->data.erase(key);
		} else {
			e = ERROR_MAPKEY;
		}
	} else {
		e = ERROR_NOTMAP;
	}
}

int DataElement::mapLength(){
	int l = 0;
	if (type==T_MAP) {
		l = map->data.size();
	} else {
		e = ERROR_NOTMAP;
	}
	return l;
}

bool DataElement::mapKey(QString key){
	// is the key in the map
	if (type==T_MAP) {
		if (map->data.count(key)) {
			return true;
		} else {
			e = ERROR_MAPKEY;
		}
	} else {
		e = ERROR_NOTMAP;
	}
	return false;
}
