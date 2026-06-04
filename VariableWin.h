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

#ifndef __VARIABLEWIN_H
#define __VARIABLEWIN_H

#define COLUMNNAME          0
#define COLUMNTYPE          1
#define COLUMNVALUE         2

#include <QtWidgets/QTreeWidget>

#include "ViewWidgetIFace.h"
#include "DataElement.h"
#include "Variables.h"
#include "Convert.h"

class VariableWin : public QTreeWidget, public ViewWidgetIFace {
    Q_OBJECT;

public:
    VariableWin();
    ~VariableWin();
    void clear();

public slots:
    void varWinAssign(Variables **, int, int);
    void varWinAssign(Variables **, int, int, int, int);
    void varWinAssign(Variables **, int, int, QString);
    void varWinDropLevel(int);

private:
	Convert *convert;
	void setTypeAndValue(QTreeWidgetItem *, DataElement *);
	QString tr_stringType;
	QString tr_integerType;
	QString tr_floatType;
	QString tr_referenceType;
	QString tr_arrayType;
	QString tr_mapType;
	QString tr_unknownType;
};


class TreeWidgetItem : public QTreeWidgetItem {
public:
    TreeWidgetItem():QTreeWidgetItem(){}

private:
    bool operator<(const QTreeWidgetItem &other)const {
        const int column = treeWidget()->sortColumn();
        if(column==COLUMNTYPE)
            return data(column,Qt::EditRole) < other.data(column,Qt::EditRole);
        return data(column,Qt::UserRole + 1) < other.data(column,Qt::UserRole + 1);
    }
};

#endif
