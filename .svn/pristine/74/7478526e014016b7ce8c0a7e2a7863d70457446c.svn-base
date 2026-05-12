#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ErrorCodes.h"
#include "Settings.h"

#include <QCoreApplication>
#include <QString>

class Error
{
	Q_DECLARE_TR_FUNCTIONS(Error)
	
	public:

		// current 
		int e;					// error
		int var;				// variable number
		int x;					// array row
		int y;					// array column
		QString extra;			// extra text
		int line;				// line number
		
		//pending
		int pending_e;
		int pending_var;
		int pending_x;
		int pending_y;
		QString pending_extra;
		
		void loadSettings();
		  
		Error();
		
		bool pending();

		void process(int);
		
		void q(int);
		void q(int, int);			// with variable
		void q(int, int, int, int);	// with array variable and dimensions
		void q(int, QString);		// with extra text
		
		void deq();

		bool isFatal();
		bool isFatal(int);

		QString getErrorMessage(char **);
		QString getErrorMessage(int, int, char**);
 		
 	private:
 		int typeconverror;
 		int varnotassignederror;
 		
 		void q(int, int, int, int, QString);

};

extern Error *error;
