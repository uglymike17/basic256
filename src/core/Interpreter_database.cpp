/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 **/

// Database opcodes.
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execDatabaseOp().  The cases are exactly as they were
// there: a break still ends the opcode, and the checks execByteCode()
// makes after every opcode still run when this returns.

#include "InterpreterPrivate.h"

void Interpreter::execDatabaseOp(int opcode) {
	switch(opcode) {

		case OP_DBOPEN: {
			// open database connection
			QString file = stack->popQString();
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
#ifdef BASIC256_ENABLE_SQL
				if (!fileSecurity.allowPath(file, tr("open the database"))) break;
				
				closeDatabase(n);
				QString dbconnection = QStringLiteral("DBCONNECTION") + QString::number(n);
				QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",dbconnection);
				db.setDatabaseName(file);
				bool ok = db.open();
				if (!ok) {
					// say why - "Driver not loaded" when the SQLite plugin is
					// missing from a package, SQLite's own reason otherwise
					QString why = db.lastError().databaseText();
					if (why.isEmpty()) why = db.lastError().driverText();
					error->q(ERROR_DBOPEN, why);
					closeDatabase(n);
				}
#else
				(void)file;
				error->q(ERROR_NOTAVAILABLE);
#endif
			}
		}
		break;

		case OP_DBCLOSE: {
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
#ifdef BASIC256_ENABLE_SQL
				closeDatabase(n);
#else
				error->q(ERROR_NOTAVAILABLE);
#endif
			}
		}
		break;

		case OP_DBEXECUTE: {
			// execute a statement on the database
			QString stmt = stack->popQString();
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
#ifdef BASIC256_ENABLE_SQL
				QString dbconnection = QStringLiteral("DBCONNECTION") + QString::number(n);
				QSqlDatabase db = QSqlDatabase::database(dbconnection);
				if(db.isValid()) {
					if (!fileSecurity.allowSql(stmt)) break;
					
					QSqlQuery *q = new QSqlQuery(db);
					bool ok = q->exec(stmt);
					if (!ok) {
						error->q(ERROR_DBQUERY, q->lastError().databaseText());
					}
					delete q;
				} else {
					error->q(ERROR_DBNOTOPEN);
				}
#else
				(void)stmt;
				error->q(ERROR_NOTAVAILABLE);
#endif
			}
		}
		break;

		case OP_DBOPENSET: {
			// open recordset
			QString stmt = stack->popQString();
			int set = stack->popInt();
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
				if (set<0||set>=NUMDBSET) {
					error->q(ERROR_DBSETNUMBER);
				} else {
#ifdef BASIC256_ENABLE_SQL
					QString dbconnection = QStringLiteral("DBCONNECTION") + QString::number(n);
					QSqlDatabase db = QSqlDatabase::database(dbconnection);
					if(db.isValid()) {
						if (!fileSecurity.allowSql(stmt)) break;
						
						if (dbSet[n][set]) {
							dbSet[n][set]->clear();
							delete dbSet[n][set];
							dbSet[n][set] = NULL;
						}
						dbSet[n][set] = new QSqlQuery(db);
						bool ok = dbSet[n][set]->exec(stmt);
						if (!ok) {
							error->q(ERROR_DBQUERY, dbSet[n][set]->lastError().databaseText());
						}
					} else {
						error->q(ERROR_DBNOTOPEN);
					}
#else
					(void)stmt;
					error->q(ERROR_NOTAVAILABLE);
#endif
				}
			}
		}
		break;

		case OP_DBCLOSESET: {
			int set = stack->popInt();
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
				if (set<0||set>=NUMDBSET) {
					error->q(ERROR_DBSETNUMBER);
				} else {
#ifdef BASIC256_ENABLE_SQL
					if (dbSet[n][set]) {
						dbSet[n][set]->clear();
						delete dbSet[n][set];
						dbSet[n][set] = NULL;
					} else {
						error->q(ERROR_DBNOTSET);
					}
#else
					error->q(ERROR_NOTAVAILABLE);
#endif
				}
			}
		}
		break;

		case OP_DBROW: {
			int set = stack->popInt();
			int n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
				if (set<0||set>=NUMDBSET) {
					error->q(ERROR_DBSETNUMBER);
				} else {
#ifdef BASIC256_ENABLE_SQL
					if (dbSet[n][set]) {
						// return true if we move to a new row else false
						stack->pushInt(dbSet[n][set]->next());
					} else {
						error->q(ERROR_DBNOTSET);
					}
#else
					error->q(ERROR_NOTAVAILABLE);
#endif
				}
			}
		}
		break;

		case OP_DBINT:
		case OP_DBFLOAT:
		case OP_DBNULL:
		case OP_DBSTRING: {

			int col = -1, set, n;
			QString colname;
			bool usename;
			if (stack->peekType()==T_STRING) {
				usename = true;
				colname = stack->popQString();
			} else {
				usename = false;
				col = stack->popInt();
			}
			set = stack->popInt();
			n = stack->popInt();
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
			} else {
				if (set<0||set>=NUMDBSET) {
					error->q(ERROR_DBSETNUMBER);
				} else {
#ifdef BASIC256_ENABLE_SQL
					// no record set was ever opened on this set number: dbSet is NULL
					if (!dbSet[n][set] || !dbSet[n][set]->isActive()) {
						error->q(ERROR_DBNOTSET);
					} else {
						if (!dbSet[n][set]->isValid()) {
							error->q(ERROR_DBNOTSETROW);
						} else {
							if (usename) {
								col = dbSet[n][set]->record().indexOf(colname);
							}
							if (col < 0 || col >= dbSet[n][set]->record().count()) {
								error->q(ERROR_DBCOLNO);
							} else if (!error->pending()){
								switch(opcode) {
									case OP_DBINT:
										stack->pushInt(dbSet[n][set]->record().value(col).toInt());
										break;
									case OP_DBFLOAT:
										// potential issue with locale and database
										// it seems like locale->toDouble does not support a QVariant
										stack->pushDouble(dbSet[n][set]->record().value(col).toDouble());
										break;
									case OP_DBNULL:
										stack->pushInt(dbSet[n][set]->record().value(col).isNull());
										break;
									case OP_DBSTRING:
										// potential issue with locale and database
										// it seems like locale->toString does not support a QVariant
										stack->pushQString(dbSet[n][set]->record().value(col).toString());
										break;
								}
							}else{
								//in case of an error
								if(opcode==OP_DBSTRING)
									stack->pushQString("");
								else
									stack->pushInt(0);
							}
						}
					}
#else
					(void)usename;
					(void)colname;
					(void)col;
					error->q(ERROR_NOTAVAILABLE);
					if(opcode==OP_DBSTRING)
						stack->pushQString("");
					else
						stack->pushInt(0);
#endif
				}
			}
		}
		break;

		case OP_FREEDB: {
			// return the next free databsae number - throw error if none free
#ifdef BASIC256_ENABLE_SQL
			int f=-1;
			for (int t=0; (t<NUMDBCONN)&&(f==-1); t++) {
				QString dbconnection = QStringLiteral("DBCONNECTION") + QString::number(t);
				QSqlDatabase db = QSqlDatabase::database(dbconnection);
				if (!db.isValid()) f = t;
			}
			if (f==-1) {
				error->q(ERROR_FREEDB);
				stack->pushInt(0);
			} else {
				stack->pushInt(f);
			}
#else
			error->q(ERROR_NOTAVAILABLE);
			stack->pushInt(0);
#endif
		}
		break;

		case OP_FREEDBSET: {
			// return the next free set for a database - throw error if none free
			int n = stack->popInt();
			int f=-1;
			if (n<0||n>=NUMDBCONN) {
				error->q(ERROR_DBCONNNUMBER);
				stack->pushInt(0);
			} else {
				for (int t=0; (t<NUMDBSET)&&(f==-1); t++) {
					if (!dbSet[n][t]) f = t;
				}
				if (f==-1) {
					error->q(ERROR_FREEDBSET);
					stack->pushInt(0);
				} else {
					stack->pushInt(f);
				}
			}
		}
		break;

	}
}
