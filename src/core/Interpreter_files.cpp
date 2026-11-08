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

// Files opcodes.
//
// Interpreter::execByteCode() in Interpreter.cpp decodes each opcode and
// hands these ones to execFileOp().  The cases are exactly as they were
// there: a break still ends the opcode, and the checks execByteCode()
// makes after every opcode still run when this returns.

#include "InterpreterPrivate.h"

void Interpreter::execFileOp(int opcode) {
	switch(opcode) {

		case OP_OPEN: {
			int type = stack->popInt();	// 0 text 1 binary
			QString name = stack->popQString();
			int fn = stack->popInt();

			if (!fileSecurity.allowPath(name, tr("open the file"))) break;
			
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				// close file number if open
				if (filehandle[fn] != NULL) {
					filehandle[fn]->close();
					filehandle[fn] = NULL;
				}
				// create filehandle
				if(name=="STDOUT") {
					QFile *tempf = new QFile();
					tempf->QFile::open(stdout, QIODevice::WriteOnly);
					filehandle[fn] = tempf;
				} else {
					filehandle[fn] = new QFile(name);
				}
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILEOPEN);
				} else {
					filehandletype[fn] = type;
					if (type==0) {
						// text file (type 0)
						if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Text)) {
							error->q(ERROR_FILEOPEN);
						}
					} else {
						// binary file (type 1)
						if (!filehandle[fn]->open(QIODevice::ReadWrite)) {
							error->q(ERROR_FILEOPEN);
						}
					}
				}
			}
		}
		break;

		case OP_OPENFILEDIALOG: {
			if (guiState == GUISTATESILENT) {
				std::cerr << "OPENFILEDIALOG not supported in --silent mode." << std::endl;
				std::exit(1);
			}
			QString filter = stack->popQString();
			QString path = stack->popQString();
			QString prompt = stack->popQString();
			mymutex->lock();
			emit(dialogOpenFileDialog(prompt, path, filter));
			waitCond->wait(mymutex);
			mymutex->unlock();
			// consent by selection: the user chose this path themselves
			if (!inputString.isEmpty()) {
				fileSecurity.rememberChosen(inputString);
			}
			stack->pushQString(inputString);
		}
		break;

		case OP_SAVEFILEDIALOG: {
			if (guiState == GUISTATESILENT) {
				std::cerr << "SAVEFILEDIALOG not supported in --silent mode." << std::endl;
				std::exit(1);
			}
			QString filter = stack->popQString();
			QString path = stack->popQString();
			QString prompt = stack->popQString();
			mymutex->lock();
			emit(dialogSaveFileDialog(prompt, path, filter));
			waitCond->wait(mymutex);
			mymutex->unlock();
			// consent by selection: the user chose this path themselves
			if (!inputString.isEmpty()) {
				fileSecurity.rememberChosen(inputString);
			}
			stack->pushQString(inputString);
		}
		break;

		case OP_READLINE: {
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
				stack->pushQString("");
			} else {

				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
					stack->pushQString("");
				} else {
					//read entire line
					filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
					stack->pushQString(QString::fromUtf8(filehandle[fn]->readLine()));
				}
			}
		}
		break;

		case OP_READBYTE: {
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
				stack->pushInt(0);
			} else if (filehandle[fn] == NULL) {
				error->q(ERROR_FILENOTOPEN);
				stack->pushInt(0);
			} else {
				char c = ' ';
				filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
				if (filehandle[fn]->getChar(&c)) {
					stack->pushInt((int) (unsigned char) c);
				} else {
					stack->pushInt((int) -1);
				}
			}
		}
		break;

		case OP_EOF: {
			//return true to eof if error is returned
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
				stack->pushInt(1);
			} else {
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
					stack->pushInt(1);
				} else {
					switch (filehandletype[fn]) {
						case 0:
						case 1:
							// normal file eof
							if (filehandle[fn]->atEnd()) {
								stack->pushInt(1);
							} else {
								stack->pushInt(0);
							}
							break;
						case 2:
							// serial
							QCoreApplication::processEvents();
							if (filehandle[fn]->bytesAvailable()==0) {
								stack->pushInt(1);
							} else {
								stack->pushInt(0);
							}
							break;
					}
				}
			}
		}
		break;

		case OP_WRITE:
		case OP_WRITELINE: {
			QString temp = stack->popQString();
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				int fileerror = 0;
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					fileerror = filehandle[fn]->write(temp.toUtf8().data());
					if (opcode == OP_WRITELINE) {
						fileerror = filehandle[fn]->putChar('\n');
					}
				   if (filehandle[fn]->isSequential()) {
						filehandle[fn]->waitForBytesWritten(FILEWRITETIMEOUT);
					}
				}
				if (fileerror == -1) {
					error->q(ERROR_FILEWRITE);
				}
			}
		}
		break;

		case OP_WRITEBYTE: {
			int n = stack->popInt();
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				int fileerror = 0;
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					fileerror = filehandle[fn]->putChar((unsigned char) n);
					if (filehandle[fn]->isSequential()) filehandle[fn]->waitForBytesWritten(FILEWRITETIMEOUT);
					if (fileerror == -1) {
						error->q(ERROR_FILEWRITE);
					}
				}
			}
		}
		break;

		case OP_CLOSE: {
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					filehandle[fn]->close();
					filehandle[fn] = NULL;
				}
			}
		}
		break;

		case OP_RESET: {
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					if (filehandle[fn]->isSequential()) {
						error->q(ERROR_FILEOPERATION);
					} else {
						switch (filehandletype[fn]) {
							case 0:
								// text mode file (close and reopen)
								filehandle[fn]->close();
								if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) {
									error->q(ERROR_FILERESET);
								}
								break;
							case 1:
								// binary mode file
								filehandle[fn]->close();
								if (!filehandle[fn]->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
									error->q(ERROR_FILERESET);
								}
								break;
							case 2:
								// serial pot
								error->q(ERROR_FILEOPERATION);
								break;
						}
					}
				}
			}
		}
		break;

		case OP_SIZE: {
			// push the current open file size on the stack
			int fn = stack->popInt();
			int size = 0;
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					switch(filehandletype[fn]) {
						case 0:
						case 1:
							// normal file
							size = filehandle[fn]->size();
							break;
						case 2:
							// serial
							QCoreApplication::processEvents();
							size = filehandle[fn]->bytesAvailable();
							break;
					}
				}
			}
			stack->pushInt(size);
		}
		break;

		case OP_EXISTS: {
			// push a 1 if file exists else zero

			QString filename = stack->popQString();
			QDir dir = QDir::current();
			if (dir.exists(filename)) {
				stack->pushInt(1);
			} else {
				stack->pushInt(0);
			}
		}
		break;

		case OP_SEEK: {
			// move file pointer to a specific loaction in file
			long pos = stack->popInt();
			int fn = stack->popInt();
			if (fn<0||fn>=NUMFILES) {
				error->q(ERROR_FILENUMBER);
			} else {
				if (filehandle[fn] == NULL) {
					error->q(ERROR_FILENOTOPEN);
				} else {
					switch(filehandletype[fn]) {
						case 0:
						case 1:
							// normal file
							filehandle[fn]->seek(pos);
							break;
						case 2:
							// serial
							error->q(ERROR_FILEOPERATION);
							break;
					}
				}
			}
		}
		break;

		case OP_CHANGEDIR: {
			QString file = stack->popQString();
			if(!QDir::setCurrent(file)) {
				error->q(ERROR_FOLDER);
			}
		}
		break;

		case OP_CURRENTDIR: {
			stack->pushQString(QDir::currentPath());
		}
		break;

		case OP_KILL: {
			QString name = stack->popQString();
			if (!fileSecurity.allowPath(name, tr("delete the file"))) break;
			
			if(!QFile::remove(name)) {
				error->q(ERROR_FILEOPEN);
			}
		}
		break;

		case OP_DIR: {
		    QString folder = stack->popQString();

    				// New folder requested: build fresh directory listing
    				if (!folder.isEmpty()) {
       				 	directory = QDir(folder);

        				if (!directory.exists()) {
            				error->q(ERROR_FOLDER);
            				stack->pushQString(QString(""));
           		 			break;
        				}

        				directoryEntries = directory.entryList(
            				QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
            				QDir::Name
        				);

        				directoryIndex = 0;
    				}

    				// Return next entry
    				if (directoryIndex < directoryEntries.size()) {
        				stack->pushQString(directoryEntries[directoryIndex++]);
    				} else {
        				stack->pushQString(QString(""));
    				}
		}
		break;

		case OP_FREEFILE: {
			// return the next free file number - throw error if not free files
			int f=-1;
			for (int t=0; (t<NUMFILES)&&(f==-1); t++) {
				if (!filehandle[t]) f = t;
			}
			if (f==-1) {
				error->q(ERROR_FREEFILE);
				stack->pushInt(0);
			} else {
				stack->pushInt(f);
			}
		}
		break;

		case OP_MKDIR: {
			QString name = stack->popQString();
			//fprintf(stderr,"mkdir %s\n",name.toUtf8().data());
			if (!fileSecurity.allowPath(name, tr("create the folder"))) break;
			
			QDir dir = QDir::current();
			if (!dir.exists(name)) {
				if(!dir.mkdir(name)) {
					error->q(ERROR_MKDIR);
				}
			}
		}
		break;


		// insert additional OPTYPE_NONE operations here

	}
}
