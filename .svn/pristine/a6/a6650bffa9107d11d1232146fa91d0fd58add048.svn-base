#include "Convert.h"
#include "Error.h"

#include <string>


int Convert::e = ERROR_NONE;

Convert::Convert(QLocale *applocale) {
	SETTINGS;
	decimaldigits = settings.value(SETTINGSDECDIGS, SETTINGSDECDIGSDEFAULT).toInt();
	if(decimaldigits < SETTINGSDECDIGSMIN || decimaldigits > SETTINGSDECDIGSMAX)
		decimaldigits = SETTINGSDECDIGSDEFAULT;
	floattail = settings.value(SETTINGSFLOATTAIL, SETTINGSFLOATTAILDEFAULT).toBool();
	replaceDecimalPoint = settings.value(SETTINGSFLOATLOCALE, SETTINGSFLOATLOCALEDEFAULT).toBool();

	// build international safe regular expression for numbers
	locale = applocale;
	replaceDecimalPoint = replaceDecimalPoint && locale->decimalPoint()!='.'; //use locale decimal point only if !="."
	decimalPoint = (replaceDecimalPoint?locale->decimalPoint():'.');
	isnumericexpression = QString("^[-+]?[0-9]*") + decimalPoint + QString("?[0-9]+([eE][-+]?[0-9]+)?$");
	musicalnote.setPattern("^(do|re|mi|fa|sol|la|si|c|d|e|f|g|a|b|h|ni|pa|vu|ga|di|ke|zo)([-]?[0-9]+)?(#{1,2}|b{1,2})?$");
	musicalnote.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

Convert::~Convert() {
}


bool Convert::isNumeric(DataElement *d) {
	// return true if this is a number or can convert to one
	// if there is white space around the string number IT IS NOT NUMERIC
	if (d) {
		if (d->type == T_INT || d->type == T_FLOAT) {
			return true;
		} else if (d->type == T_STRING) {
			if (d->stringval.length()!=0) {
				// using isnumeric as private (like musicalnote) increase time for all functions
				// eg. getBool() increased time elapsed from 10200ms to 12500ms
				// Benchmark: convert->getBool() for 2.700.000.000 times
				// ...I have no explanation for this...
				QRegularExpression isnumeric(isnumericexpression);
				QRegularExpressionMatch match = isnumeric.match(d->stringval);
				return match.hasMatch();
			}
		}
	}
	return false;
}


int Convert::getBool(DataElement *d) {
	// BASIC56 uses 1 for true and 0 for false
	// a number not zero is 1 otherwise 0
	// a non empty string is 1 otherwise 0
	// an unassigned variable is 0
	// an array throws an error and is 0
	if (d) {
		if (d->type == T_INT) {
			return (d->intval!=0l);
		} else if (d->type == T_FLOAT) {
			return ((d->floatval < -EPSILON || d->floatval > EPSILON)?1:0);
		} else if (d->type == T_STRING) {
			return ((d->stringval.length()!=0)?1:0);
		} else {
			// anything else  = false
			e = ERROR_BOOLEANCONV;
			return 0;
		}
	}
	return 0;
}

int Convert::getInt(DataElement *d) {
	long l=getLong(d);
	if (l<INT_MIN||l>INT_MAX) {
		e = ERROR_INTEGERRANGE;
		l = 0;
	}
	return (int) l;
}

long Convert::getLong(DataElement *d) {
	long i=0;
	if (d) {
		if (d->type == T_INT) {
			i = d->intval;
		} else if (d->type == T_FLOAT) {
			double f = d->floatval + (d->floatval>0.0?EPSILON:-EPSILON);
			if (f<LONG_MIN||f>LONG_MAX) {
				e = ERROR_LONGRANGE;
			} else {
				i = (long) f;
			}
		} else if (d->type == T_STRING) {
			if (d->stringval.length()!=0) {
				bool ok;
				i = d->stringval.toLong(&ok);
				if(!ok) {
					i = 0;
					e = ERROR_NUMBERCONV;
				}
			} else {
				e = ERROR_NUMBERCONV;
			}
		} else{
			e = ERROR_NUMBERCONV;
		}
	}
	return i;
}

double Convert::getFloat(DataElement *d) {
	double f = 0.0;
	if (d) {
		if (d->type == T_FLOAT) {
			return(d->floatval);
		} else if (d->type == T_INT) {
			return(d->intval);
		} else if (d->type == T_STRING) {
			if (d->stringval.length()!=0) {
				bool ok;
				if(replaceDecimalPoint){
					f = locale->toDouble(d->stringval, &ok);
				}else{
					f = d->stringval.toDouble(&ok);
				}
				if(!ok) {
					f = 0.0;
					e = ERROR_NUMBERCONV;
				}
			}else {
				e = ERROR_NUMBERCONV;
			}
		} else {
			e = ERROR_NUMBERCONV;
		}
	}
	return f;
}

double Convert::getMusicalNote(DataElement *d) {
	double f = 0.0;
	if (d) {
		if (d->type == T_FLOAT) {
			f = d->floatval;
		} else if (d->type == T_INT) {
			f = d->intval;
		}else if (d->type == T_STRING) {
			QRegularExpressionMatch match = musicalnote.match(d->stringval);
			if (match.hasMatch()) {
				QString note, octave, accidentals;
				int oct = 4, acc = 0, n;
				double s;
				note = match.captured(1).toLower();
				n = notesmap[note];
				octave = match.captured(2);
				if(octave!="") oct = octave.toInt();
				accidentals = match.captured(3).toLower();
				if(accidentals=="#") acc = 1;
				if(accidentals=="##") acc = 2;
				if(accidentals=="b") acc = -1;
				if(accidentals=="bb") acc = -2;

				s=oct*12+n+acc-57; //if s==0 then s==440Hz
				f=pow(2.0,s/12.0)*440;
			}else{
				if (d->stringval.length()!=0) {
					bool ok;
					if(replaceDecimalPoint){
						f = locale->toDouble(d->stringval, &ok);
					}else{
						f = d->stringval.toDouble(&ok);
					}
					if(!ok) {
						f = 0.0;
						e = ERROR_STRING2NOTE;
					}
				}else {
					e = ERROR_STRING2NOTE;
				}
			}
			} else {
				e = ERROR_STRINGCONV;
			}
		}
	return f;
}

QString Convert::getString(DataElement *d) {
	return getString(d, decimaldigits);
}

QString Convert::getString(DataElement *d, int ddigits) {
	QString s = "";
	if (d) {
		if (d->type == T_STRING) {
			s = d->stringval;
		} else if (d->type == T_INT) {
			s = QString::number(d->intval);
		} else if (d->type == T_FLOAT) {
			if(d->floatval >= -EPSILON && d->floatval <= EPSILON){
				//this is almost 0.0
				//If you add 0.1/-0.1 you will never get a perfect 0.0 again
				if(floattail)
					s = QStringLiteral("0") + decimalPoint + QStringLiteral("0");
				else
					s = QStringLiteral("0");
			}else{
				double xp = log10(d->floatval*(d->floatval<0.0?-1.0:1.0)); // size in powers of 10
				//check if adding of ".0" will exceed the number of digits to print numbers
				if(((int)xp)==ddigits-1 && floattail){
					s.setNum(d->floatval,'e',ddigits - 1);
					s.replace(QRegExp(QStringLiteral("0+e")), QStringLiteral("e"));
					s.replace(QStringLiteral(".e"), QStringLiteral(".0e"));
					if(replaceDecimalPoint){
						s.replace('.', decimalPoint);
					}
				}else if (xp*2<-ddigits || (int)xp>=(ddigits-(floattail?1:0))) { //number is too small or too big to show in original form
					s.setNum(d->floatval,'g',ddigits);
					if(replaceDecimalPoint){
						s.replace('.', decimalPoint);
					}
				} else {
					int dig = ddigits - (xp>0.0?(int)xp+1:1);
					//double precision sucks
					//if we need to print more than 7 decimals there are chances to print numbers as 2.300000001 instead of 2.3
					if(dig>7){
						s.setNum(d->floatval,'f',7);
						if(!s.endsWith(QStringLiteral("0"))) // we cannot reduce it
							s.setNum(d->floatval,'f',dig);
					}else{
						s.setNum(d->floatval,'f',dig);
					}
					if (s.contains('.',Qt::CaseInsensitive)) {
						while(s.endsWith(QStringLiteral("0"))) s.chop(1);
						if(s.endsWith(QStringLiteral("."))){
							s.chop(1);
							if(floattail)
								s.append(decimalPoint + '0');
						}else if(replaceDecimalPoint){
							s.replace('.', decimalPoint);
						}
					}else if(floattail){
						s.append(decimalPoint + '0');
					}
				}
			}
		} else {
			e = ERROR_STRINGCONV;
		}
	}
	return s;
}


int Convert::compareFloats(double fone, double ftwo) {
	// return 1 if one>two  0 if one==two or -1 if one<two
	// USE FOR ALL COMPARISON WITH FLOATS
	// used a small number (epsilon) to make sure that
	// decimal precission errors are ignored
	if (fabs(fone - ftwo)<=EPSILON) return 0;
	else if (fone < ftwo) return -1;
	else return 1;
}

int Convert::compare(DataElement *one, DataElement* two) {
	// complex compare logic - compare two stack types with each other
	// return 1 if one>two  0 if one==two or -1 if one<two
	//
	if (one&&two) {
		if (one->type == T_INT && two->type == T_INT) {
			// if both are integers comapre as integers
			long ans = one->intval - two->intval;
			if (ans==0) return 0;
			else if (ans<0) return -1;
			else return 1;
		} else if (one->type == T_STRING && two->type == T_STRING) {
			// if both are strings - [compare them as strings] strcmp
			int ans = one->stringval.compare(two->stringval);
			if (ans==0) return 0;
			else if (ans<0) return -1;
			else return 1;
		} else {
			// if either a float number then compare as floats
			double ftwo = getFloat(two);
			double fone = getFloat(one);
			return compareFloats(fone, ftwo);
		}
	} else {
		return 0;
	}
}

QPolygonF *Convert::getPolygonF(DataElement *d) {
	// returns a QPolygonF - used in several graphics OPcodes
	QPolygonF *poly = new QPolygonF();
	if (d->type==T_ARRAY) {
		if (d->arrayRows()==1 || d->arrayCols()==2) {
			if (d->arrayRows()*d->arrayCols()>=6) {
				if (d->arrayRows()==1) {
					for(int col = 0; col < d->arrayCols(); col+=2){
						poly->append(QPointF(getFloat(d->arrayGetData(0,col)), getFloat(d->arrayGetData(0,col+1))));
					}
				} else {
					for(int row = 0; row < d->arrayRows(); row++){
						poly->append(QPointF(getFloat(d->arrayGetData(row,0)), getFloat(d->arrayGetData(row,1))));
					}
				}
			} else {
				e = ERROR_POLYPOINTS;
			}
		} else {
			e = ERROR_ARRAYEVEN;
		}
	} else {
		e = ERROR_ARRAYEXPR;
	}
	//fprintf(stderr,"getPolygon %d %d %d\n", d->arrayRows(), d->arrayCols(), poly->size());
	return poly;
}
