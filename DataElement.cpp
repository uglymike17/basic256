#include "DataElement.h"
#include "Settings.h"

#include <string>

int DataElement::e = ERROR_NONE;

void DataElement::init() {
	// initialize dataelement common stuff
	e=0;
	type = T_UNASSIGNED;
	arr = NULL;
	map = NULL;
}

DataElement::DataElement() {
	// create an empty dataelement
	init();
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


DataElement::DataElement(DataElement *de) {
	init();
	copy(de);
}

DataElement::~DataElement() {
	clear();
}

void DataElement::copy(DataElement *source) {
	// fill an existing from as a copy of another
	// it is as fast as it can be
	// unused values of strings can be found as garbage but this is the cost of speed
	// source->stringval.clear() is too expensive to be used for each assignment/copy
	clear();
	if (source) {
		type = source->type;
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
						arr->data[i] = new DataElement(source->arr->data[i]);
					}
				}
				break;
			case T_MAP:
				{
#ifdef DEBUG
fprintf(stderr,"de copy map source len %d\n",source->map->data.size());
#endif
					if (!map)
						map = new DataElementMap;
					for (std::map<std::string, DataElement*>::iterator it = source->map->data.begin(); it != source->map->data.end(); it++ ) {
						mapSetData(it->first, it->second);
					}
				}
				break;
			case T_REF:
				intval = source->intval;
				level = source->level;
				break;
			case T_INT:
				intval = source->intval;
		}
	}
}

void DataElement::clear() {
	//fprintf(stderr, "DataElement clear %s\n", debug().toStdString().c_str());
	switch (type){
		//case T_STRING:
		//	stringval.clear();
		//	break;
		case T_ARRAY:
			if (arr) {
				int i = arr->xdim * arr->ydim;
				while(i-- > 0) {
					if (arr->data[i]) delete arr->data[i];
					arr->data[i] = NULL;
				}
				arr->data.clear();
				delete(arr);
				arr = NULL;
			}
			break;
		case T_MAP:
			if (map) {
				for (std::map<std::string, DataElement*>::iterator it = map->data.begin(); it != map->data.end(); it++ ) {
					if (it->second) delete it->second;
					it->second = NULL;
				}
				map->data.clear();
				delete(map);
				map = NULL;
			}
			break;
	}
	type = T_UNASSIGNED;
}

int DataElement::getType(DataElement* e) {
	if (e) {
		return e->type;
	} else {
		return T_UNASSIGNED;
	}
}

int DataElement::getError() {
	return getError(false);
}

int DataElement::getError(int clear) {
	int olde = e;
	if (clear) e = ERROR_NONE;
	return olde;
}

QString DataElement::debug() {
	// return a string representing the DataElement contents
	if(type==T_INT) return("int(" + QString::number(intval) + ") ");
	if(type==T_REF) return("varref(" + QString::number(intval) + ", level: " + QString::number(level) + ") ");
	if(type==T_FLOAT) return("float(" + QString::number(floatval) + ") ");
	if(type==T_STRING) return("string(" + stringval + ") ");
	if(type==T_ARRAY) return("array(" + QString::number(intval) + ")");
	if(type==T_MAP) return("MAP("+ QString::number(map->data.size()) + ")");
	if(type==T_UNASSIGNED) return("unused ");
	return("badtype(" + QString::number(type) + ")");
}

void DataElement::arrayDim(const int xdim, const int ydim, const bool redim) {
	const int size = xdim * ydim;
	
	if (size <= MAXARRAYSIZE) {
		if (size >= 1) {
			if (type != T_ARRAY || !redim) {
				// if array data is dim or redim without a dim then create a new one (clear the old)
				clear();
				arr = new DataElementArray;
				arr->data.resize(size, NULL);
				type = T_ARRAY;
			}else{
				// redim - resize the vector
				arr->data.resize(size, NULL);
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
	// DO NOT DELETE ****** COPY OF THE DATAELEMENT INTERNAL STORAGE
	DataElement *d;
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			if (arr->data[i]) {
				return arr->data[i];
			} else {
				e = ERROR_VARNOTASSIGNED;
			}
		} else {
			e = ERROR_ARRAYINDEX;
		}
	} else {
		e = ERROR_NOTARRAY;
	}
	return NULL;
}

void DataElement::arraySetData(const int x, const int y, DataElement *d) {
	// DataElement's data is copied and it should be deleted
	// by whomever created it
	if (type == T_ARRAY) {
		if (x >=0 && x < arr->xdim && y >=0 && y < arr->ydim) {
			const int i = x * arr->ydim + y;
			if (!arr->data[i]) arr->data[i] = new DataElement();
			arr->data[i]->copy(d);
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
			if (arr->data[i])  {
				arr->data[i]->clear();
			}
		} else {
			e = ERROR_ARRAYINDEX;
		}
	} else {
		e = ERROR_NOTARRAY;
	}
}



void DataElement::mapDim(){
	clear();
	map = new DataElementMap();
	type=T_MAP;
}

DataElement* DataElement::mapGetData(QString qkey){
	// DO NOT DELETE ****** COPY OF THE DATAELEMENT INTERNAL STORAGE
	DataElement *d;
	if (type==T_MAP) {
		std::string key = qkey.toUtf8().constData();
		//fprintf(stdout, "mapGetData key = %s\n", key.c_str());
		if (map->data.count(key)) {
			return map->data[key];
		} else {
			e = ERROR_MAPKEY;
		}
	} else {
		e = ERROR_NOTMAP;
	}
	return NULL;
}

void DataElement::mapSetData(QString qkey, DataElement *d){
	mapSetData(qkey.toStdString(), d);
}

void DataElement::mapSetData(std::string key, DataElement *d){
	if (type==T_MAP) {
		if (!map->data.count(key)) {
			map->data[key] = new DataElement();
		}
		map->data[key]->copy(d);
	} else {
		e = ERROR_NOTMAP;
	}
}

void DataElement::mapUnassign(QString qkey){
	if (type==T_MAP) {
		std::string key = qkey.toUtf8().constData();
		if (map->data.count(key)) {
			delete map->data[key];
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

bool DataElement::mapKey(QString qkey){
	// is the key in the map
	if (type==T_MAP) {
		std::string key = qkey.toUtf8().constData();
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
