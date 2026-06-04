/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

#include "VariableWin.h"
#include "DataElement.h"
#include "MainWindow.h"

extern MainWindow * mainwin;

extern "C" {
extern char **symtable;
extern int numsyms;
}

VariableWin::VariableWin () {
    setColumnCount(3);
    setHeaderLabels(QStringList() << tr("Level - Name") << tr("Ty") << tr("Value"));
    resizeColumnToContents(COLUMNTYPE);
    sortByColumn(COLUMNNAME, Qt::AscendingOrder);
    setSortingEnabled(true);
    convert = new Convert(mainwin->locale);
    //hold translations to speed up things
    tr_stringType = tr("Type: String");
    tr_integerType = tr("Type: Integer");
    tr_floatType = tr("Type: Float");
    tr_arrayType = tr("Type: Array");
    tr_referenceType = tr("Type: Reference");
    tr_mapType = tr("Type: Map");
    tr_unknownType = tr("?");
}

VariableWin::~VariableWin() {
    delete(convert);
}

void VariableWin::setTypeAndValue(QTreeWidgetItem *r, DataElement *d) {
	// set thr type and value columns (2 and 3)
	QString var;
	if (r) {
		r->takeChildren();
		switch(DataElement::getType(d)) {
			case T_STRING:{
				r->setText(COLUMNTYPE, QStringLiteral("S"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_stringType);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, d->stringval);
				r->setText(COLUMNVALUE, d->stringval);
				break;
			}
			case T_INT:{
				r->setText(COLUMNTYPE, QStringLiteral("I"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_integerType);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, (long long)d->intval);
				r->setText(COLUMNVALUE, convert->getString(d)); //to keep user choice about printing numbers
				break;
			}
			case T_FLOAT:{
				r->setText(COLUMNTYPE, QStringLiteral("F"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_floatType);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, d->floatval);
				r->setText(COLUMNVALUE, convert->getString(d)); //using user locale format
				break;
			}
			case T_REF:{
				r->setText(COLUMNTYPE, QStringLiteral("REF"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_referenceType);
				var = QString::number(d->level) + QStringLiteral(" - ") + symtable[d->intval];
				r->setText(COLUMNVALUE, var);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, var); //to do:00level+var
				break;
			}
			case T_ARRAY:{
				r->setText(COLUMNTYPE, QStringLiteral("A"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_arrayType);
				if (d->arrayRows()==1) {
					// 1d array
					var = QStringLiteral("[") + QString::number(d->arrayCols()) + QStringLiteral("]");
				} else {
					// 2d array
					var = QStringLiteral("[") + QString::number(d->arrayRows()) + QStringLiteral(",") + QString::number(d->arrayCols()) + QStringLiteral("]");
				}
				r->setText(COLUMNVALUE, var);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, var); //to do:00level+var
				break;
			}
			case T_MAP:{
				r->setText(COLUMNTYPE, QStringLiteral("M"));
				r->setData(COLUMNTYPE, Qt::ToolTipRole, tr_mapType);
				var = QStringLiteral("[") + QString::number(d->mapLength()) + QStringLiteral("]");
				r->setText(COLUMNVALUE, var);
				r->setData(COLUMNVALUE, Qt::UserRole + 1, var); //to do:00level+var
				break;
			}
		   default:{
				r->setText(COLUMNTYPE, tr_unknownType);
				r->setData(COLUMNTYPE, Qt::ToolTipRole, QStringLiteral(""));
				r->setText(COLUMNVALUE, QStringLiteral(""));
				r->setData(COLUMNVALUE, Qt::UserRole + 1, QVariant());
			}
		}

	}
}

void VariableWin::varWinAssign(Variables **variables, int varnum, int level, int x, int y) {
	// refresh an array's element
	QTreeWidgetItem *parentItem = NULL;
	QTreeWidgetItem *childItem = NULL;
	QString parentname, childname;
	DataElement *d;
	QList<QTreeWidgetItem *> list;

	if(*variables){
		Variable *v = (*variables)->getAt(varnum, level);
		parentname = QString::number(level) + QStringLiteral(" - ") + symtable[varnum];
		// see if v is on the list and change value or add
		list = findItems(parentname, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);

		if (list.size() > 0) {
			parentItem = (TreeWidgetItem *)list[0];
			// now find child or add
			if (v->data->arrayRows()==1) {
				childname = parentname + QStringLiteral("[") + QString::number(y) + QStringLiteral("]");
			} else {
				childname = parentname + QStringLiteral("[") + QString::number(x) + QStringLiteral(",") + QString::number(y) + QStringLiteral("]");
			}
			list = findItems(childname, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);
			if (list.size() > 0) {
				// get existing element
				childItem = (TreeWidgetItem *)list[0];
			} else {
				// create a new one
				childItem = new TreeWidgetItem();
				childItem->setText(COLUMNNAME, childname);
				childItem->setData(COLUMNNAME, Qt::UserRole + 1, childname);
				parentItem->addChild(childItem);
			}
			setTypeAndValue(childItem, v->data->arrayGetData(x,y));
		}
	}
}

void VariableWin::varWinAssign(Variables **variables, int varnum, int level, QString key) {
	// refresh a map's elements
	QTreeWidgetItem *parentItem = NULL;
	QTreeWidgetItem *childItem = NULL;
	QString parentname, childname;
	DataElement *d;
	QList<QTreeWidgetItem *> list;

	if(*variables){
		Variable *v = (*variables)->getAt(varnum, level);
		parentname = QString::number(level) + QStringLiteral(" - ") + symtable[varnum];
		// see if v is on the list and change value or add
		list = findItems(parentname, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);

		if (list.size() > 0) {
			parentItem = (TreeWidgetItem *)list[0];
			// now find child or add
			childname = parentname + QStringLiteral("[") + key + QStringLiteral("]");
			list = findItems(childname, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);
			if (list.size() > 0) {
				// get existing element
				childItem = (TreeWidgetItem *)list[0];
			} else {
				// create a new one
				childItem = new TreeWidgetItem();
				childItem->setText(COLUMNNAME, childname);
				childItem->setData(COLUMNNAME, Qt::UserRole + 1, childname);
				parentItem->addChild(childItem);
			}
			setTypeAndValue(childItem, v->data->mapGetData(key));
		}
	}
}

void VariableWin::varWinAssign(Variables **variables, int varnum, int level) {
	// refresh variable/array content
	TreeWidgetItem *rowItem = NULL;
	QString name;

	if(*variables){
		Variable *v = (*variables)->getAt(varnum, level);
		name = QString::number(level) + QStringLiteral(" - ") + symtable[varnum];

		// see if v is on the list and change value or add
		QList<QTreeWidgetItem *> list = findItems(name, Qt::MatchExactly | Qt::MatchRecursive, COLUMNNAME);

		if (list.size() > 0) {
			// get existing element
			rowItem = (TreeWidgetItem *)list[0];
		} else {
			// create a new one
			rowItem = new TreeWidgetItem();
			rowItem->setText(COLUMNNAME, name);
			QString id = QString("%1").arg(v->data->level, 7, 10, QChar('0')) + symtable[varnum];
			rowItem->setData(COLUMNNAME, Qt::UserRole + 1, id);
			addTopLevelItem(rowItem);
		}
		setTypeAndValue(rowItem, v->data);
		//
		// if an array set initial values if any
		if (v->data->type==T_ARRAY) {
			for(int x = 0; x<v->data->arr->xdim;x++) {
				for(int y = 0; y<v->data->arr->ydim;y++) {
					DataElement *q = v->data->arrayGetData(x,y);
					DataElement::getError(true);
					if (q && q->type!=T_UNASSIGNED) {
						varWinAssign(variables, varnum, level, x, y);
					}
				}  
			}  
		}
		//
		// if a map set initial values if any
		if (v->data->type==T_MAP) {
			for (std::map<std::string, DataElement*>::iterator it = v->data->map->data.begin(); it != v->data->map->data.end(); it++ ) {
				varWinAssign(variables, varnum, level, QString::fromStdString(it->first));
			}
		}
	}
}

void VariableWin::varWinDropLevel(int level) {
    // when we return from a subroutine or function delete its variables
    QString name = QString::number(level) + QStringLiteral(" ");
    QList<QTreeWidgetItem *> items = findItems(name, Qt::MatchStartsWith, COLUMNNAME);
    for (int n=items.size()-1; n>=0; n--) {
        delete items[n];
    }
}

void VariableWin::clear() {
    // when window is cleared (Interpreter restarts) we reload convert settings
    // to make sure that convert will reflect the current settings (user may change those any time)
    delete (convert);
    convert = new Convert(mainwin->locale);
    QTreeWidget::clear();
}
