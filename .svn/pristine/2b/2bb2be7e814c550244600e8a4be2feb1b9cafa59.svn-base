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

%{
//#define YYDEBUG 1

	#ifdef __cplusplus
		extern "C" {
	#endif

	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <math.h>
	#include <errno.h>
	#include "../BasicTypes.h"
	#include "../Constants.h"
	#include "../WordCodes.h"
	#include "../CompileErrors.h"
	#include "../ErrorCodes.h"
	#include "../Version.h"


	#define SYMTABLESIZE 2000
	#define IFTABLESIZE 1000
	#define PARSEWARNINGTABLESIZE 10

	extern int yylex();
	extern char *yytext;
	int yyerror(const char *);
	int errorcode;
	extern int column;
	extern int linenumber;
	extern char *lexingfilename;
	extern int numincludes;
	extern int filenumber;
	extern char* include_filenames[];
	extern int include_filenames_counter;

	int *wordCode = NULL;
	unsigned int maxwordoffset = 0;		// size of the current wordCode array
	unsigned int wordOffset = 0;		// current location on the WordCode array

	unsigned int listlen = 0;
	unsigned int listlenmax = 0;
	unsigned int numberoflists = 0;

	unsigned int varnumber[IFTABLESIZE];	// stack of variable numbers in a statement to return the varmumber
	int nvarnumber=0;


	int functionDefSymbol = -1;	// if in a function definition (what is the symbol number) -1 = not in fundef
	int subroutineDefSymbol = -1;	// if in a subroutine definition (what is the symbol number) -1 = not in fundef

	struct label
	{
		char *name;
		int offset;
	};

	char *EMPTYSTR = "";
        char **symtable=NULL;
        int *symtableaddress=NULL;
        int *symtableaddresstype=NULL;
        int *symtableaddressargs=NULL;
        int numsyms = 0;
        int maxsymtable = 0; // size of the current symtable/symtableaddress/symtableaddresstype arrays



	// array to hold stack of if statement branch locations
	// that need to have final jump location added to them
	// the iftable is also used by for, subroutine, and function to insure
	// that no if,do,while,... is nested incorrectly
	unsigned int iftablesourceline[IFTABLESIZE];
	unsigned int iftabletype[IFTABLESIZE];
	int iftableid[IFTABLESIZE];			// used to store a sequential number for this if - unique label creation
	int iftableincludes[IFTABLESIZE];			// used to store the include depth of the code
	unsigned int iftablevariable[IFTABLESIZE];			// store the variable in a FOR to check at NEXT
	int numifs = 0;
	int nextifid;

	#define IFTABLETYPEIF 1
	#define IFTABLETYPEELSE 2
	#define IFTABLETYPEDO 3
	#define IFTABLETYPEWHILE 4
	#define IFTABLETYPEFOR 5
	#define IFTABLETYPEFUNCTION 6
	#define IFTABLETYPETRY 7
	#define IFTABLETYPECATCH 8
	#define IFTABLETYPEBEGINCASE 9
	#define IFTABLETYPECASE 10
	#define IFTABLETYPESUBROUTINE 11
	#define IFTABLETYPEINTERNAL 12


	// store the function variables here during a function definition
	unsigned int args[100];
	unsigned int argstype[100];
	int numargs = 0;

	#define ARGSTYPEVALUE 0
        #define ARGSTYPEVARARRAY 1

	// compiler workings - store in array so that interperter can display all of them
	int parsewarningtable[PARSEWARNINGTABLESIZE];
	int parsewarningtablelinenumber[PARSEWARNINGTABLESIZE];
	int parsewarningtablecolumn[PARSEWARNINGTABLESIZE];
        int parsewarningtablelexingfilenumber[PARSEWARNINGTABLESIZE];
	int numparsewarnings = 0;

        int basicParse(char *);

	void checkWordMem(unsigned int addedwords) {
		unsigned int t;
		if (wordOffset + addedwords + 1 >= maxwordoffset) {
                        maxwordoffset = maxwordoffset + addedwords + 2048;
			wordCode = realloc(wordCode, maxwordoffset * sizeof(int));
			for (t=wordOffset; t<maxwordoffset; t++) {
				*(wordCode+t) = 0;
                        }
		}
	}

	int bytesToFullWords(unsigned int size) {
		// return how many words will be needed to store "size" bytes
                return((size + sizeof(int) - 1) / sizeof(int));
	}

	void addOp(int op) {
		checkWordMem(1);
		wordCode[wordOffset] = op;
                wordOffset++;
		//printf("line=%i addOp op=%i\n",linenumber, op);
	}

        void addData(int data) {
                checkWordMem(1);
                wordCode[wordOffset] = data;
                wordOffset++;
        }

	void addIntOp(int op, int data) {
		addOp(op);
                addData(data);
	}

	void addIntIntOp(int op, int data, int data2) {
		addOp(op);
		addData(data);
		addData(data2);
	}

	void addFloatOp(int op, double data) {
		addOp(op);
		unsigned int wlen = bytesToFullWords(sizeof(double));
		checkWordMem(wlen);
		double *temp = (double *) (wordCode + wordOffset);
		*temp = data;
		wordOffset += wlen;
	}

	void addStringOp(int op, char *data) {
		addOp(op);
		unsigned int len = strlen(data) + 1;
		unsigned int wlen = bytesToFullWords(len);
		checkWordMem(wlen);
		strncpy((char *) (wordCode + wordOffset), data, len);
		wordOffset += wlen;
	}

	void clearIfTable() {
		int j;
		for (j = 0; j < IFTABLESIZE; j++) {
			iftablesourceline[j] = -1;
			iftabletype[j] = -1;
			iftableid[j] = -1;
			iftableincludes[j] = -1;
		}
		numifs = 0;
		nextifid = 0;
	}

	int testIfOnTable(int includelevel) {
		// return line number if there is an unfinished while.do.if.else
		// or send back -1
		if (numifs >=1 ) {
			if (iftableincludes[numifs-1]>=includelevel) {
				return iftablesourceline[numifs-1];
			}
		}
		return -1;
	}

	int testIfOnTableError(int includelevel) {
		// return Error number if there is an unfinished while.do.if.else
		// or send back 0
		if (numifs >=1 ) {
			if (iftableincludes[numifs-1]>=includelevel) {
				if (iftabletype[numifs-1]==IFTABLETYPEIF) return COMPERR_IFNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEELSE) return COMPERR_ELSENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEDO) return COMPERR_DONOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEWHILE) return COMPERR_WHILENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEFOR) return COMPERR_FORNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) return COMPERR_FUNCTIONNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPETRY) return COMPERR_TRYNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPECATCH) return COMPERR_CATCHNOEND;
				if (iftabletype[numifs-1]==IFTABLETYPEBEGINCASE) return COMPERR_BEGINCASENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPECASE) return COMPERR_CASENOEND;
				if (iftabletype[numifs-1]==IFTABLETYPESUBROUTINE) return COMPERR_SUBROUTINENOEND;
			}
		}
		return 0;
	}

	int newIf(int sourceline, int type, unsigned int variable) {
		iftablesourceline[numifs] = sourceline;
		iftabletype[numifs] = type;
		iftableid[numifs] = nextifid;
		iftableincludes[numifs] = numincludes;
		iftablevariable[numifs] = variable;
		nextifid++;
		numifs++;
		return numifs - 1;
	}

	void newParseWarning(int type) {
		// add warning to warnings table (if not maximum)
		if (numparsewarnings<PARSEWARNINGTABLESIZE) {
			parsewarningtable[numparsewarnings] = type;
			parsewarningtablelinenumber[numparsewarnings] = linenumber;
			parsewarningtablecolumn[numparsewarnings] = column;
                        parsewarningtablelexingfilenumber[numparsewarnings] = filenumber;
			numparsewarnings++;
		} else {
			parsewarningtable[numparsewarnings-1] = COMPWARNING_MAXIMUMWARNINGS;
			parsewarningtablelinenumber[numparsewarnings-1] = 0;
			parsewarningtablecolumn[numparsewarnings-1] = 0;
                        parsewarningtablelexingfilenumber[numparsewarnings-1] = 0;
		}
	}

	int getSymbol(char *name) {
		// get a symbol if it exists or create a new one on the symbol table
		int i;
		for (i = 0; i < numsyms; i++) {
                        if (symtable[i] && !strcasecmp(name, symtable[i]))
				return i;
		}

		//allocate memory if there is no more room for new symbol
		if(numsyms>=maxsymtable-1){
			maxsymtable += 1024;
			symtable = realloc(symtable, maxsymtable * sizeof(char*));
			symtableaddress = realloc(symtableaddress, maxsymtable * sizeof(int));
			symtableaddresstype = realloc(symtableaddresstype, maxsymtable * sizeof(int));
			symtableaddressargs = realloc(symtableaddressargs, maxsymtable * sizeof(int));
		}

		symtable[numsyms] = strdup(name);
		symtableaddress[numsyms] = -1;
		symtableaddresstype[numsyms] = -1;
		symtableaddressargs[numsyms] = -1;
		numsyms++;
		return numsyms - 1;
	}

	#define INTERNALSYMBOLEXIT 0 //at the end of the loop - all done
	#define INTERNALSYMBOLCONTINUE 1 //at the test of the loop
        #define INTERNALSYMBOLTOP 2 // at the top of the loop - all done

	int getInternalSymbol(int id, int type) {
		// an internal symbol used to jump an if
		int i;
		char name[32];
		sprintf(name,"___%d_%d", id, type);
		i = getSymbol(name);
		symtableaddresstype[i]=ADDRESSTYPE_SYSTEMCALL;
		return i;
	}

	void freeBasicParse() {
		// free all dynamically allocated stuff
		while(numsyms>0) free(symtable[--numsyms]);
		free(wordCode);
		wordCode=NULL;
		free(symtable);
		symtable=NULL;
		free(symtableaddress);
		symtableaddress=NULL;
		free(symtableaddresstype);
		symtableaddresstype=NULL;
		free(symtableaddressargs);
		symtableaddressargs=NULL;
		maxsymtable = 0;
		maxwordoffset = 0;

		while(include_filenames_counter>0){
			include_filenames_counter--;
			free(include_filenames[include_filenames_counter]);
		}
	}

	int initializeBasicParse() {
		int f;
		maxsymtable = 2048;
		symtable = malloc(maxsymtable * sizeof(char*));
		if(symtable)
			for(f=0;f<maxsymtable;f++) symtable[f]=NULL;
		symtableaddress = malloc(maxsymtable * sizeof(int));
		symtableaddresstype = malloc(maxsymtable * sizeof(int));
		symtableaddressargs = malloc(maxsymtable * sizeof(int));

		maxwordoffset = 2048;
		wordCode = malloc(maxwordoffset * sizeof(int));

		//no memory
		if(!wordCode || !symtable || !symtableaddress || !symtableaddresstype || !symtableaddressargs){
			freeBasicParse();
			return -1;
		}

		unsigned int t=maxwordoffset;
		while(t>0) wordCode[--t] = 0;
		wordOffset = 0;
		linenumber = 1;

		listlen = 0;
		listlenmax = 0;
		numberoflists = 0;

		addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
		return 0; 	// success in creating and filling
	}


	#ifdef __cplusplus
	}
	#endif

%}

/* Generate the parser description file. */
%verbose


%token B256ABS
%token B256ACOS
%token B256ADD1
%token B256ADDEQUAL
%token B256ALERT
%token B256AMP
%token B256AMPEQUAL
%token B256AND
%token B256ARC
%token B256ARRAYBASE
%token B256ASC
%token B256ASIN
%token B256ASSIGNED
%token B256ATAN
%token B256BEGINCASE
%token B256BINARYNOT
%token B256BINARYOR
%token B256BITSHIFTL
%token B256BITSHIFTR
%token B256BLACK
%token B256BLUE
%token B256BOOLEOF
%token B256BOOLFALSE
%token B256BOOLTRUE
%token B256CALL
%token B256CASE
%token B256CATCH
%token B256CEIL
%token B256CHANGEDIR
%token B256CHORD
%token B256CHR
%token B256CIRCLE
%token B256CLEAR
%token B256CLG
%token B256CLICKB
%token B256CLICKCLEAR
%token B256CLICKX
%token B256CLICKY
%token B256CLOSE
%token B256CLS
%token B256CONFIRM
%token B256CONTINUEDO
%token B256CONTINUEFOR
%token B256CONTINUEWHILE
%token B256COS
%token B256COUNT
%token B256COUNTX
%token B256CURRENTDIR
%token B256CYAN
%token B256DARKBLUE
%token B256DARKCYAN
%token B256DARKGREEN
%token B256DARKGREY
%token B256DARKORANGE
%token B256DARKPURPLE
%token B256DARKRED
%token B256DARKYELLOW
%token B256DAY
%token B256DBCLOSE
%token B256DBCLOSESET
%token B256DBEXECUTE
%token B256DBFLOAT
%token B256DBINT
%token B256DBNULL
%token B256DBOPEN
%token B256DBOPENSET
%token B256DBROW
%token B256DBSTRING
%token B256DEBUGINFO
%token B256DEGREES
%token B256DIM
%token B256DIR
%token B256DIVEQUAL
%token B256DO
%token B256EDITVISIBLE
%token B256ELLIPSE
%token B256ELSE
%token B256END
%token B256ENDCASE
%token B256ENDFUNCTION
%token B256ENDIF
%token B256ENDSUBROUTINE
%token B256ENDTRY
%token B256ENDWHILE
%token B256ERROR_ARGUMENTCOUNT
%token B256ERROR_ARRAYELEMENT
%token B256ERROR_ARRAYEVEN
%token B256ERROR_ARRAYEXPR
%token B256ERROR_ARRAYINDEX
%token B256ERROR_ARRAYINDEXMISSING
%token B256ERROR_ARRAYLENGTH2D
%token B256ERROR_ARRAYNITEMS
%token B256ERROR_ARRAYSIZELARGE
%token B256ERROR_ARRAYSIZESMALL
%token B256ERROR_ASINACOSRANGE
%token B256ERROR_BOOLEANCONV
%token B256ERROR_DBCOLNO
%token B256ERROR_DBCONNNUMBER
%token B256ERROR_DBNOTOPEN
%token B256ERROR_DBNOTSET
%token B256ERROR_DBNOTSETROW
%token B256ERROR_DBOPEN
%token B256ERROR_DBQUERY
%token B256ERROR_DBSETNUMBER
%token B256ERROR_DIVZERO
%token B256ERROR_DOWNLOAD
%token B256ERROR_ENVELOPEMAX
%token B256ERROR_ENVELOPEODD
%token B256ERROR_EXPECTEDARRAY
%token B256ERROR_EXPECTEDSOUND
%token B256ERROR_FILENOTOPEN
%token B256ERROR_FILENUMBER
%token B256ERROR_FILEOPEN
%token B256ERROR_FILEOPERATION
%token B256ERROR_FILERESET
%token B256ERROR_FILEWRITE
%token B256ERROR_FOLDER
%token B256ERROR_FREEDB
%token B256ERROR_FREEDBSET
%token B256ERROR_FREEFILE
%token B256ERROR_FREENET
%token B256ERROR_HARMONICLIST
%token B256ERROR_HARMONICNUMBER
%token B256ERROR_IMAGEFILE
%token B256ERROR_IMAGERESOURCE
%token B256ERROR_IMAGESAVETYPE
%token B256ERROR_IMAGESCALE
%token B256ERROR_INFINITY
%token B256ERROR_INTEGERRANGE
%token B256ERROR_INVALIDKEYNAME
%token B256ERROR_INVALIDPROGNAME
%token B256ERROR_INVALIDRESOURCE
%token B256ERROR_LOGRANGE
%token B256ERROR_LONGRANGE
%token B256ERROR_MAXRECURSE
%token B256ERROR_NETACCEPT
%token B256ERROR_NETBIND
%token B256ERROR_NETCONN
%token B256ERROR_NETHOST
%token B256ERROR_NETNONE
%token B256ERROR_NETREAD
%token B256ERROR_NETSOCK
%token B256ERROR_NETSOCKNUMBER
%token B256ERROR_NETSOCKOPT
%token B256ERROR_NETWRITE
%token B256ERROR_NEXTNOFOR
%token B256ERROR_NONE
%token B256ERROR_NOSUCHFUNCTION
%token B256ERROR_NOSUCHLABEL
%token B256ERROR_NOSUCHSUBROUTINE
%token B256ERROR_NOTARRAY
%token B256ERROR_NOTIMPLEMENTED
%token B256ERROR_NUMBERCONV
%token B256ERROR_NUMBEREXPR
%token B256ERROR_ONEDIMENSIONAL
%token B256ERROR_ONERRORSUB
%token B256ERROR_PENWIDTH
%token B256ERROR_PERMISSION
%token B256ERROR_POLYPOINTS
%token B256ERROR_PRINTERNOTOFF
%token B256ERROR_PRINTERNOTON
%token B256ERROR_PRINTEROPEN
%token B256ERROR_RADIX
%token B256ERROR_RADIXSTRING
%token B256ERROR_REFNOTASSIGNED
%token B256ERROR_RGB
%token B256ERROR_SERIALPARAMETER
%token B256ERROR_SETTINGMAXKEYS
%token B256ERROR_SETTINGMAXLEN
%token B256ERROR_SETTINGSGETACCESS
%token B256ERROR_SETTINGSSETACCESS
%token B256ERROR_SLICESIZE
%token B256ERROR_SOUNDERROR
%token B256ERROR_SOUNDFILE
%token B256ERROR_SOUNDFILEFORMAT
%token B256ERROR_SOUNDLENGTH
%token B256ERROR_SOUNDNOTSEEKABLE
%token B256ERROR_SOUNDRESOURCE
%token B256ERROR_SPRITENA
%token B256ERROR_SPRITENUMBER
%token B256ERROR_SPRITESLICE
%token B256ERROR_SQRRANGE
%token B256ERROR_STACKUNDERFLOW
%token B256ERROR_STRING2NOTE
%token B256ERROR_STRINGCONV
%token B256ERROR_STRINGEXPR
%token B256ERROR_STRINGMAXLEN
%token B256ERROR_STRSTART
%token B256ERROR_TOOMANYSOUNDS
%token B256ERROR_UNEXPECTEDRETURN
%token B256ERROR_UNSERIALIZEFORMAT
%token B256ERROR_VARCIRCULAR
%token B256ERROR_VARNOTASSIGNED
%token B256ERROR_VARNULL
%token B256ERROR_WAVEFORMLOGICAL
%token B256ERROR_WAVOBSOLETE
%token B256EXISTS
%token B256EXITDO
%token B256EXITFOR
%token B256EXITWHILE
%token B256EXP
%token B256EXPLODE
%token B256EXPLODEX
%token B256FASTGRAPHICS
%token B256FILL
%token B256FLOOR
%token B256FONT
%token B256FOR
%token B256FOREACH
%token B256FREEDB
%token B256FREEDBSET
%token B256FREEFILE
%token B256FREENET
%token B256FROMBINARY
%token B256FROMHEX
%token B256FROMOCTAL
%token B256FROMRADIX
%token B256FUNCTION
%token B256GETARRAYBASE
%token B256GETBRUSHCOLOR
%token B256GETCLIPBOARDIMAGE
%token B256GETCLIPBOARDSTRING
%token B256GETCOLOR
%token B256GETPENWIDTH
%token B256GETSETTING
%token B256GETSLICE
%token B256GLOBAL
%token B256GOSUB
%token B256GOTO
%token B256GRAPHHEIGHT
%token B256GRAPHSIZE
%token B256GRAPHVISIBLE
%token B256GRAPHTOOLBARVISIBLE
%token B256GRAPHWIDTH
%token B256GREEN
%token B256GREY
%token B256GTE
%token B256HOUR
%token B256IF
%token B256IN
%token B256IMAGEAUTOCROP
%token B256IMAGECENTERED
%token B256IMAGECOPY
%token B256IMAGECROP
%token B256IMAGEDRAW
%token B256IMAGEFLIP
%token B256IMAGEHEIGHT
%token B256IMAGELOAD
%token B256IMAGENEW
%token B256IMAGEPIXEL
%token B256IMAGERESIZE
%token B256IMAGEROTATE
%token B256IMAGESETPIXEL
%token B256IMAGESMOOTH
%token B256IMAGETRANSFORMED
%token B256IMAGETYPE_BMP
%token B256IMAGETYPE_JPG
%token B256IMAGETYPE_PNG
%token B256IMAGEWIDTH
%token B256IMGLOAD
%token B256IMGSAVE
%token B256IMPLODE
%token B256INPUT
%token B256INPUTFLOAT
%token B256INPUTINT
%token B256INPUTSTRING
%token B256INSTR
%token B256INSTRX
%token B256INTDIV
%token B256ISNUMERIC
%token B256KEY
%token B256KEYPRESSED
%token B256KILL
%token B256LASTERROR
%token B256LASTERROREXTRA
%token B256LASTERRORLINE
%token B256LASTERRORMESSAGE
%token B256LEFT
%token B256LENGTH
%token B256LET
%token B256LINE
%token B256LJUST
%token B256LOG
%token B256LOGTEN
%token B256LOWER
%token B256LTE
%token B256LTRIM
%token B256MAINTOOLBARVISIBLE
%token B256MAP
%token B256MD5
%token B256MID
%token B256MIDX
%token B256MINUTE
%token B256MOD
%token B256MONTH
%token B256MOUSEB
%token B256MOUSEBUTTON_CENTER
%token B256MOUSEBUTTON_DOUBLECLICK
%token B256MOUSEBUTTON_LEFT
%token B256MOUSEBUTTON_NONE
%token B256MOUSEBUTTON_RIGHT
%token B256MOUSEX
%token B256MOUSEY
%token B256MSEC
%token B256MULEQUAL
%token B256NE
%token B256NETADDRESS
%token B256NETCLOSE
%token B256NETCONNECT
%token B256NETDATA
%token B256NETLISTEN
%token B256NETREAD
%token B256NETWRITE
%token B256NEXT
%token B256NOT
%token B256OFFERROR
%token B256ONERROR
%token B256OPEN
%token B256OPENB
%token B256OPENFILEDIALOG
%token B256OPENSERIAL
%token B256OR
%token B256ORANGE
%token B256OSTYPE
%token B256OSTYPE_ANDROID
%token B256OSTYPE_LINUX
%token B256OSTYPE_MACINTOSH
%token B256OSTYPE_WINDOWS
%token B256OUTPUTVISIBLE
%token B256OUTPUTTOOLBARVISIBLE
%token B256PAUSE
%token B256PENWIDTH
%token B256PI
%token B256PIE
%token B256PIXEL
%token B256PLOT
%token B256POLY
%token B256PORTIN
%token B256PORTOUT
%token B256PRINT
%token B256PRINTERCANCEL
%token B256PRINTEROFF
%token B256PRINTERON
%token B256PRINTERPAGE
%token B256PROMPT
%token B256PURPLE
%token B256PUTSLICE
%token B256RADIANS
%token B256RAND
%token B256READ
%token B256READBYTE
%token B256READLINE
%token B256RECT
%token B256RED
%token B256REDIM
%token B256REF
%token B256REFRESH
%token B256REGEXMINIMAL
%token B256REM
%token B256REPLACE
%token B256REPLACEX
%token B256SAVEFILEDIALOG
%token B256RESET
%token B256RETURN
%token B256RGB
%token B256RIGHT
%token B256RJUST
%token B256ROUND
%token B256RTRIM
%token B256SAY
%token B256SECOND
%token B256SEED
%token B256SEEK
%token B256SEMICOLON
%token B256SEMICOLONEQUAL
%token B256SERIALIZE
%token B256SETCLIPBOARDIMAGE
%token B256SETCLIPBOARDSTRING
%token B256SETCOLOR
%token B256SETGRAPH
%token B256SETSETTING
%token B256SIN
%token B256SIZE
%token B256SLICE_ALL
%token B256SLICE_PAINT
%token B256SLICE_SPRITE
%token B256SOUND
%token B256SOUNDENVELOPE
%token B256SOUNDFADE
%token B256SOUNDHARMONICS
%token B256SOUNDID
%token B256SOUNDLENGTH
%token B256SOUNDLOAD
%token B256SOUNDLOADRAW
%token B256SOUNDLOOP
%token B256SOUNDPAUSE
%token B256SOUNDPLAY
%token B256SOUNDPLAYER
%token B256SOUNDPLAYEROFF
%token B256SOUNDPOSITION
%token B256SOUNDSAMPLERATE
%token B256SOUNDSEEK
%token B256SOUNDSTATE
%token B256SOUNDSTOP
%token B256SOUNDSYSTEM
%token B256SOUNDVOLUME
%token B256SOUNDWAIT
%token B256SOUNDWAVEFORM
%token B256SPRITECOLLIDE
%token B256SPRITEDIM
%token B256SPRITEH
%token B256SPRITEHIDE
%token B256SPRITELOAD
%token B256SPRITEMOVE
%token B256SPRITEO
%token B256SPRITEPLACE
%token B256SPRITEPOLY
%token B256SPRITER
%token B256SPRITES
%token B256SPRITESHOW
%token B256SPRITESLICE
%token B256SPRITEV
%token B256SPRITEW
%token B256SPRITEX
%token B256SPRITEY
%token B256SQR
%token B256STAMP
%token B256STEP
%token B256SUB1
%token B256SUBEQUAL
%token B256SUBROUTINE
%token B256SYSTEM
%token B256TAN
%token B256TEXT
%token B256TEXTHEIGHT
%token B256TEXTWIDTH
%token B256THEN
%token B256THROWERROR
%token B256TO
%token B256TOBINARY
%token B256TOFLOAT
%token B256TOHEX
%token B256TOINT
%token B256TOOCTAL
%token B256TORADIX
%token B256TOSTRING
%token B256TRIM
%token B256TRY
%token B256TYPEOF
%token B256TYPE_ARRAY
%token B256TYPE_FLOAT
%token B256TYPE_INT
%token B256TYPE_MAP
%token B256TYPE_REF
%token B256TYPE_STRING
%token B256TYPE_UNASSIGNED
%token B256UNASSIGN
%token B256UNLOAD
%token B256UNSERIALIZE
%token B256UNTIL
%token B256UPPER
%token B256VARIABLEWATCH
%token B256VERSION
%token B256VOLUME
%token B256WARNING_ARRAYELEMENT
%token B256WARNING_BOOLEANCONV
%token B256WARNING_INTEGERRANGE
%token B256WARNING_LONGRANGE
%token B256WARNING_NUMBERCONV
%token B256WARNING_REFNOTASSIGNED
%token B256WARNING_SOUNDERROR
%token B256WARNING_SOUNDFILEFORMAT
%token B256WARNING_SOUNDLENGTH
%token B256WARNING_SOUNDNOTSEEKABLE
%token B256WARNING_START
%token B256WARNING_STRING2NOTE
%token B256WARNING_STRINGCONV
%token B256WARNING_VARNOTASSIGNED
%token B256WARNING_WAVOBSOLETE
%token B256WAVLENGTH
%token B256WAVPAUSE
%token B256WAVPLAY
%token B256WAVPOS
%token B256WAVSEEK
%token B256WAVSTATE
%token B256WAVSTOP
%token B256WAVWAIT
%token B256WHILE
%token B256WHITE
%token B256WRITE
%token B256WRITEBYTE
%token B256WRITELINE
%token B256XOR
%token B256YEAR
%token B256YELLOW
%token B256ZFILL

%union anytype {
	int number;
	double floatnum;
	char *string;
}

%token <number> B256INTEGER
%token <floatnum> B256FLOAT
%token <string> B256STRING
%token <string> B256HEXCONST
%token <string> B256BINCONST
%token <string> B256OCTCONST
%token <number> B256VARIABLE
%token <number> B256LABEL


%right ','
%left B256IN
%left B256SEMICOLON
%left B256XOR B256OR
%left B256AND
%nonassoc B256NOT B256ADD1 B256SUB1
%left '<' B256LTE '>' B256GTE '=' B256NE
%left B256BINARYOR B256AMP
%left B256BITSHIFTL B256BITSHIFTR
%left '-' '+'
%left '*' '/' B256MOD B256INTDIV
%nonassoc B256UNARY B256BINARYNOT
%left '^'
%left '[' ']'
%left '{' '}'
%left '(' ')'



%%

program:
	programline programnewline program
	| programline
	;

programnewline:
	'\n' {
		linenumber++;
		column=0;
		addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
	}
	;

programline:
	label compoundstmt
	| label compoundstmt B256REM
	| compoundstmt
	| compoundstmt B256REM
	| label
	| label B256REM
	| B256REM
	| /* empty */
	;

label:
	B256LABEL {
		if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
			errorcode = COMPERR_FUNCTIONGOTO;
			return -1;
		}
		//
		//check if name of label is already used by a function, subroutine or another label
		if (symtableaddress[$1] != -1) {
			errorcode = COMPERR_LABELREDEFINED;
			return -1;
		}
		symtableaddress[$1] = wordOffset;
		symtableaddresstype[$1] = ADDRESSTYPE_LABEL;
	}
	;
	
	
compoundstmt:
	statement ':' compoundstmt
	| statement
	;
	
	
/* ***************************************************** */
/* * define a variable, empty array, and variable with * */
/* * indexing operators following.                     * */
/* * used in expressions to extract values to the      * */
/* * stack and in statements to assign values          * */
/* ***************************************************** */


array_none:
	'[' ']'
	;

array_size:
	'[' '?' ']'
	;
	
array_size_rows:
	'[' '?' ',' ']'
	| array_size array_none
	;
	
array_size_cols:
	'[' ',' '?' ']'
	| array_none array_size
	;
	
array_indexing:
	/* array reference - make everything 2d */
	'[' expr ',' expr ']'
	| '[' expr ']' '[' expr ']'
	| '[' expr ']' {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_STACKSWAP);
	}
	;

variable:
	B256VARIABLE {
		varnumber[nvarnumber++] = $1;
	}
	| '(' variable ')'
	;

variable_a:
	variable
	| variable array_none
	| '(' variable_a ')'
	;

array_element:
	variable array_indexing
	| '(' array_element ')'
	;


	
/* ***************************************************** */
/* * define function variables                         * */
/* ***************************************************** */

functionvariable:
		variable_a {
			args[numargs] = varnumber[--nvarnumber]; argstype[numargs] = ARGSTYPEVALUE; numargs++;
			//printf("functionvariable %i %i %i\n", args[numargs-1], argstype[numargs-1],numargs);
		}
		;

functionvariablelist:
		'(' ')'
		| '(' functionvariables ')'
		;


functionvariables:
	functionvariable
	| functionvariable ',' functionvariables
	;


/* ***************************************************** */
/* * lists and lists of lists used with various        * */
/* * statements and calling functions                  * */
/* *                                                   * */
/* *                                                   * */
/* ***************************************************** */

// a mustached list of mustached lists (2 dimensional array)
listoflists:
	'{' listinlist '}'{
		addIntOp(OP_PUSHINT, numberoflists);		// number of lists (y dim)
		addIntOp(OP_PUSHINT, listlenmax);			// maximum number of expressions (x dim)
#ifdef DEBUG
	fprintf(stderr, "listlenmax %d\n", listlenmax);
#endif
		numberoflists = 0;
		listlenmax=0;
	}

// child of list of lists representing a single row of values
listinlist:
	listitems {
		addIntOp(OP_PUSHINT, listlen);
		if (listlen>listlenmax) listlenmax=listlen;
		listlen = 0;
		numberoflists = 1;
	}
	| listofitems {numberoflists = 1; }
	| listofitems ',' listinlist {numberoflists++;}
	;

// a comma separated list of expressions with mustaches
// a one dimensional array
listofitems:
	'{' listitems '}' {
		addIntOp(OP_PUSHINT, listlen);
		if (listlen>listlenmax) listlenmax=listlen;
		listlen = 0;
	}
	;

// a comma sepetated list of expressions
listitems:
	expr { listlen = 1; }
	| expr ',' listitems {listlen++;}
	;

// a comma separated list of expressions with mustaches
// a one dimensional array
listofmapitems:
	'{' mapitems '}' {
		addIntOp(OP_PUSHINT, listlen);
		if (listlen>listlenmax) listlenmax=listlen;
		listlen = 0;
		numberoflists = 0;
		listlenmax=0;
	}
	;

// a comma sepetated list of expressions
mapitems:
	expr '-' '>' expr { listlen = 1; }
	| expr '-' '>' expr ',' mapitems {listlen++;}
	;
	
	
/* USED ONLY IN CALLING Functions and subroutines */
callexprlist:
	callexpr { listlen = 1; }
	| callexpr ',' callexprlist {listlen++;}
	;


/* USED ONLY IN CALLING Functions and subroutines */
callexpr:
	expr
	| B256REF '(' variable ')' { addIntOp(OP_VAR_REF, varnumber[--nvarnumber]); }
	;
	


/* ***************************************************** */
/* * statement argument patterns                       * */
/* * used to simplify defining a statement or          * */
/* * functions arguments                               * */
/* *                                                   * */
/* ***************************************************** */	


args_none:
	'(' ')'
	| /* empty */
	;


/* two arguments */

args_ee:
	expr ',' expr
	| '(' args_ee ')';

/* only used by input statement */
args_ea:
	expr ',' array_element
	|'(' args_ea ')';

/* only used by input statement */
args_ev:
	expr ',' variable
	|'(' args_ev ')';

/* three arguments */

args_eee:
	expr ',' expr ',' expr
	| '(' args_eee ')';

/* four arguments */

args_eeee:
	expr ',' expr ',' expr ',' expr
	| '(' args_eeee ')';

/* five arguments */

args_eeeee:
	expr ',' expr ',' expr ',' expr ',' expr
	| '(' args_eeeee ')';

/* six arguments */

args_eeeeee:
	expr ',' expr ',' expr ',' expr ',' expr ',' expr
	| '(' args_eeeeee ')';

/* seven arguments */

args_eeeeeee:
	expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr
	| '(' args_eeeeeee ')';

/* nine arguments */

args_eeeeeeeee:
	expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr
	| '(' args_eeeeeeeee ')';

/* ten arguments */

args_eeeeeeeeee:
	expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr
	| '(' args_eeeeeeeeee ')';



expr:
	expr_multi
	| expr_function
	| expr_constants
	| expr_errors
	| expr_numeric
	| expr_string
	| expr_dataelement
	| array_element {
		addIntOp(OP_ARR_GET, varnumber[--nvarnumber]);
	}
	;


/* ###########################################
   ### expressions of many types           ###
   ########################################### */
expr_multi:
	'(' expr ')'
	| expr '+' expr {
		addOp(OP_ADD);
	}
	| expr B256SEMICOLON expr {
		addOp(OP_CONCATENATE);
	}
	| expr B256AMP expr {
		addOp(OP_BINARYAND);
	}
	| expr '*' expr {
		addOp(OP_MUL);
	}	;

/* ###########################################
   ### Function expressions                ###
   ########################################### */
expr_function:
	variable '(' callexprlist ')' {
		// function call with arguments
		addIntOp(OP_PUSHINT, listlen); //push number of arguments passed to compare with FUNCTION definition
		addIntOp(OP_CALLFUNCTION, varnumber[--nvarnumber]);
		addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
	}
	| variable '(' ')' {
		// function call without arguments
		addIntOp(OP_PUSHINT, 0); //push number of arguments passed to compare with FUNCTION definition
		addIntOp(OP_CALLFUNCTION, varnumber[--nvarnumber]);
		addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
	}

	;



/* ###########################################
   ### Constants                           ###
   ########################################### */
expr_constants:
	B256BLACK args_none { addIntOp(OP_PUSHINT, 0xff000000); }
	| B256BLUE args_none { addIntOp(OP_PUSHINT, 0xff0000ff); }
	| B256BOOLFALSE args_none { addIntOp(OP_PUSHINT, 0); }
	| B256BOOLTRUE args_none { addIntOp(OP_PUSHINT, 1); }
	| B256CLEAR args_none { addIntOp(OP_PUSHINT, 0x00); }
	| B256CYAN args_none { addIntOp(OP_PUSHINT, 0xff00ffff); }
	| B256DARKBLUE args_none { addIntOp(OP_PUSHINT, 0xff000080); }
	| B256DARKCYAN args_none { addIntOp(OP_PUSHINT, 0xff008080); }
	| B256DARKGREEN args_none { addIntOp(OP_PUSHINT, 0xff008000); }
	| B256DARKGREY args_none { addIntOp(OP_PUSHINT, 0xff808080); }
	| B256DARKORANGE args_none { addIntOp(OP_PUSHINT, 0xffb03d00); }
	| B256DARKPURPLE args_none { addIntOp(OP_PUSHINT, 0xff800080); }
	| B256DARKRED args_none { addIntOp(OP_PUSHINT, 0xff800000); }
	| B256DARKYELLOW args_none { addIntOp(OP_PUSHINT, 0xff808000); }
	| B256GREEN args_none { addIntOp(OP_PUSHINT, 0xff00ff00); }
	| B256GREY args_none { addIntOp(OP_PUSHINT, 0xffa4a4a4); }
	| B256MOUSEBUTTON_CENTER args_none { addIntOp(OP_PUSHINT, MOUSEBUTTON_CENTER); }
	| B256MOUSEBUTTON_DOUBLECLICK args_none { addIntOp(OP_PUSHINT, MOUSEBUTTON_DOUBLECLICK); }
	| B256MOUSEBUTTON_LEFT args_none { addIntOp(OP_PUSHINT, MOUSEBUTTON_LEFT); }
	| B256MOUSEBUTTON_NONE args_none { addIntOp(OP_PUSHINT, MOUSEBUTTON_NONE); }
	| B256MOUSEBUTTON_RIGHT args_none { addIntOp(OP_PUSHINT, MOUSEBUTTON_RIGHT); }
	| B256ORANGE args_none { addIntOp(OP_PUSHINT, 0xffff6600); }
	| B256OSTYPE_ANDROID args_none { addIntOp(OP_PUSHINT, OSTYPE_ANDROID); }
	| B256OSTYPE_LINUX args_none { addIntOp(OP_PUSHINT, OSTYPE_LINUX); }
	| B256OSTYPE_MACINTOSH args_none { addIntOp(OP_PUSHINT, OSTYPE_MACINTOSH); }
	| B256OSTYPE_WINDOWS args_none { addIntOp(OP_PUSHINT, OSTYPE_WINDOWS); }
	| B256PURPLE args_none { addIntOp(OP_PUSHINT, 0xffff00ff); }
	| B256RED args_none { addIntOp(OP_PUSHINT, 0xffff0000); }
	| B256SLICE_ALL args_none { addIntOp(OP_PUSHINT, SLICE_ALL); }
	| B256SLICE_PAINT args_none { addIntOp(OP_PUSHINT, SLICE_PAINT); }
	| B256SLICE_SPRITE args_none { addIntOp(OP_PUSHINT, SLICE_SPRITE); }
	| B256TYPE_ARRAY args_none { addIntOp(OP_PUSHINT, T_ARRAY); }
	| B256TYPE_FLOAT args_none { addIntOp(OP_PUSHINT, T_FLOAT); }
	| B256TYPE_INT args_none { addIntOp(OP_PUSHINT, T_INT); }
	| B256TYPE_MAP args_none { addIntOp(OP_PUSHINT, T_MAP); }
	| B256TYPE_REF args_none { addIntOp(OP_PUSHINT, T_REF); }
	| B256TYPE_STRING args_none { addIntOp(OP_PUSHINT, T_STRING); }
	| B256TYPE_UNASSIGNED args_none { addIntOp(OP_PUSHINT, T_UNASSIGNED); }
	| B256WHITE args_none { addIntOp(OP_PUSHINT, 0xffffffff); }
	| B256YELLOW args_none { addIntOp(OP_PUSHINT, 0xffffff00); }
	
	
	
/* ###########################################
   ### Error and Warning Numbers           ###
   ########################################### */
expr_errors:
	B256ERROR_ARGUMENTCOUNT args_none {
		addIntOp(OP_PUSHINT, ERROR_ARGUMENTCOUNT);
	}
	| B256ERROR_ARRAYELEMENT args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYELEMENT);
	}
	| B256ERROR_ARRAYEVEN args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYEVEN);
	}
	| B256ERROR_ARRAYEXPR args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYEXPR);
	}
	| B256ERROR_ARRAYINDEX args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYINDEX);
	}
	| B256ERROR_ARRAYINDEXMISSING args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYINDEXMISSING);
	}
	| B256ERROR_ARRAYLENGTH2D args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYLENGTH2D);
	}
	| B256ERROR_ARRAYNITEMS args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYNITEMS);
	}
	| B256ERROR_ARRAYSIZELARGE args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYSIZELARGE);
	}
	| B256ERROR_ARRAYSIZESMALL args_none {
		addIntOp(OP_PUSHINT, ERROR_ARRAYSIZESMALL);
	}
	| B256ERROR_ASINACOSRANGE args_none {
		addIntOp(OP_PUSHINT, ERROR_ASINACOSRANGE);
	}
	| B256ERROR_BOOLEANCONV args_none {
		addIntOp(OP_PUSHINT, ERROR_BOOLEANCONV);
	}
	| B256ERROR_DBCOLNO args_none {
		addIntOp(OP_PUSHINT, ERROR_DBCOLNO);
	}
	| B256ERROR_DBCONNNUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_DBCONNNUMBER);
	}
	| B256ERROR_DBNOTOPEN args_none {
		addIntOp(OP_PUSHINT, ERROR_DBNOTOPEN);
	}
	| B256ERROR_DBNOTSET args_none {
		addIntOp(OP_PUSHINT, ERROR_DBNOTSET);
	}
	| B256ERROR_DBNOTSETROW args_none {
		addIntOp(OP_PUSHINT, ERROR_DBNOTSETROW);
	}
	| B256ERROR_DBOPEN args_none {
		addIntOp(OP_PUSHINT, ERROR_DBOPEN);
	}
	| B256ERROR_DBQUERY args_none {
		addIntOp(OP_PUSHINT, ERROR_DBQUERY);
	}
	| B256ERROR_DBSETNUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_DBSETNUMBER);
	}
	| B256ERROR_DIVZERO args_none {
		addIntOp(OP_PUSHINT, ERROR_DIVZERO);
	}
	| B256ERROR_DOWNLOAD args_none {
		addIntOp(OP_PUSHINT, ERROR_DOWNLOAD);
	}
	| B256ERROR_ENVELOPEMAX args_none {
		addIntOp(OP_PUSHINT, ERROR_ENVELOPEMAX);
	}
	| B256ERROR_ENVELOPEODD args_none {
		addIntOp(OP_PUSHINT, ERROR_ENVELOPEODD);
	}
	| B256ERROR_EXPECTEDARRAY args_none {
		addIntOp(OP_PUSHINT, ERROR_EXPECTEDARRAY);
	}
	| B256ERROR_EXPECTEDSOUND args_none {
		addIntOp(OP_PUSHINT, ERROR_EXPECTEDSOUND);
	}
	| B256ERROR_FILENOTOPEN args_none {
		addIntOp(OP_PUSHINT, ERROR_FILENOTOPEN);
	}
	| B256ERROR_FILENUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_FILENUMBER);
	}
	| B256ERROR_FILEOPEN args_none {
		addIntOp(OP_PUSHINT, ERROR_FILEOPEN);
	}
	| B256ERROR_FILEOPERATION args_none {
		addIntOp(OP_PUSHINT, ERROR_FILEOPERATION);
	}
	| B256ERROR_FILERESET args_none {
		addIntOp(OP_PUSHINT, ERROR_FILERESET);
	}
	| B256ERROR_FILEWRITE args_none {
		addIntOp(OP_PUSHINT, ERROR_FILEWRITE);
	}
	| B256ERROR_FOLDER args_none {
		addIntOp(OP_PUSHINT, ERROR_FOLDER);
	}
	| B256ERROR_FREEDB args_none {
		addIntOp(OP_PUSHINT, ERROR_FREEDB);
	}
	| B256ERROR_FREEDBSET args_none {
		addIntOp(OP_PUSHINT, ERROR_FREEDBSET);
	}
	| B256ERROR_FREEFILE args_none {
		addIntOp(OP_PUSHINT, ERROR_FREEFILE);
	}
	| B256ERROR_FREENET args_none {
		addIntOp(OP_PUSHINT, ERROR_FREENET);
	}
	| B256ERROR_HARMONICLIST args_none {
		addIntOp(OP_PUSHINT, ERROR_HARMONICLIST);
	}
	| B256ERROR_HARMONICNUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_HARMONICNUMBER);
	}
	| B256ERROR_IMAGEFILE args_none {
		addIntOp(OP_PUSHINT, ERROR_IMAGEFILE);
	}
	| B256ERROR_IMAGERESOURCE args_none {
		addIntOp(OP_PUSHINT, ERROR_IMAGERESOURCE);
	}
	| B256ERROR_IMAGESAVETYPE args_none {
		addIntOp(OP_PUSHINT, ERROR_IMAGESAVETYPE);
	}
	| B256ERROR_IMAGESCALE args_none {
		addIntOp(OP_PUSHINT, ERROR_IMAGESCALE);
	}
	| B256ERROR_INFINITY args_none {
		addIntOp(OP_PUSHINT, ERROR_INFINITY);
	}
	| B256ERROR_INTEGERRANGE args_none {
		addIntOp(OP_PUSHINT, ERROR_INTEGERRANGE);
	}
	| B256ERROR_INVALIDKEYNAME args_none {
		addIntOp(OP_PUSHINT, ERROR_INVALIDKEYNAME);
	}
	| B256ERROR_INVALIDPROGNAME args_none {
		addIntOp(OP_PUSHINT, ERROR_INVALIDPROGNAME);
	}
	| B256ERROR_INVALIDRESOURCE args_none {
		addIntOp(OP_PUSHINT, ERROR_INVALIDRESOURCE);
	}
	| B256ERROR_LOGRANGE args_none {
		addIntOp(OP_PUSHINT, ERROR_LOGRANGE);
	}
	| B256ERROR_LONGRANGE args_none {
		addIntOp(OP_PUSHINT, ERROR_LONGRANGE);
	}
	| B256ERROR_MAXRECURSE args_none {
		addIntOp(OP_PUSHINT, ERROR_MAXRECURSE);
	}
	| B256ERROR_NETACCEPT args_none {
		addIntOp(OP_PUSHINT, ERROR_NETACCEPT);
	}
	| B256ERROR_NETBIND args_none {
		addIntOp(OP_PUSHINT, ERROR_NETBIND);
	}
	| B256ERROR_NETCONN args_none {
		addIntOp(OP_PUSHINT, ERROR_NETCONN);
	}
	| B256ERROR_NETHOST args_none {
		addIntOp(OP_PUSHINT, ERROR_NETHOST);
	}
	| B256ERROR_NETNONE args_none {
		addIntOp(OP_PUSHINT, ERROR_NETNONE);
	}
	| B256ERROR_NETREAD args_none {
		addIntOp(OP_PUSHINT, ERROR_NETREAD);
	}
	| B256ERROR_NETSOCK args_none {
		addIntOp(OP_PUSHINT, ERROR_NETSOCK);
	}
	| B256ERROR_NETSOCKNUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_NETSOCKNUMBER);
	}
	| B256ERROR_NETSOCKOPT args_none {
		addIntOp(OP_PUSHINT, ERROR_NETSOCKOPT);
	}
	| B256ERROR_NETWRITE args_none {
		addIntOp(OP_PUSHINT, ERROR_NETWRITE);
	}
	| B256ERROR_NEXTNOFOR args_none {
		addIntOp(OP_PUSHINT, ERROR_NEXTNOFOR);
	}
	| B256ERROR_NONE args_none {
		addIntOp(OP_PUSHINT, ERROR_NONE);
	}
	| B256ERROR_NOSUCHFUNCTION args_none {
		addIntOp(OP_PUSHINT, ERROR_NOSUCHFUNCTION);
	}
	| B256ERROR_NOSUCHLABEL args_none {
		addIntOp(OP_PUSHINT, ERROR_NOSUCHLABEL);
	}
	| B256ERROR_NOSUCHSUBROUTINE args_none {
		addIntOp(OP_PUSHINT, ERROR_NOSUCHSUBROUTINE);
	}
	| B256ERROR_NOTARRAY args_none {
		addIntOp(OP_PUSHINT, ERROR_NOTARRAY);
	}
	| B256ERROR_NOTIMPLEMENTED args_none {
		addIntOp(OP_PUSHINT, ERROR_NOTIMPLEMENTED);
	}
	| B256ERROR_NUMBERCONV args_none {
		addIntOp(OP_PUSHINT, ERROR_NUMBERCONV);
	}
	| B256ERROR_NUMBEREXPR args_none {
		addIntOp(OP_PUSHINT, ERROR_NUMBEREXPR);
	}
	| B256ERROR_ONEDIMENSIONAL args_none {
		addIntOp(OP_PUSHINT, ERROR_ONEDIMENSIONAL);
	}
	| B256ERROR_ONERRORSUB args_none {
		addIntOp(OP_PUSHINT, ERROR_ONERRORSUB);
	}
	| B256ERROR_PENWIDTH args_none {
		addIntOp(OP_PUSHINT, ERROR_PENWIDTH);
	}
	| B256ERROR_PERMISSION args_none {
		addIntOp(OP_PUSHINT, ERROR_PERMISSION);
	}
	| B256ERROR_POLYPOINTS args_none {
		addIntOp(OP_PUSHINT, ERROR_POLYPOINTS);
	}
	| B256ERROR_PRINTERNOTOFF args_none {
		addIntOp(OP_PUSHINT, ERROR_PRINTERNOTOFF);
	}
	| B256ERROR_PRINTERNOTON args_none {
		addIntOp(OP_PUSHINT, ERROR_PRINTERNOTON);
	}
	| B256ERROR_PRINTEROPEN args_none {
		addIntOp(OP_PUSHINT, ERROR_PRINTEROPEN);
	}
	| B256ERROR_RADIX args_none {
		addIntOp(OP_PUSHINT, ERROR_RADIX);
	}
	| B256ERROR_RADIXSTRING args_none {
		addIntOp(OP_PUSHINT, ERROR_RADIXSTRING);
	}
	| B256ERROR_REFNOTASSIGNED args_none {
		addIntOp(OP_PUSHINT, ERROR_REFNOTASSIGNED);
	}
	| B256ERROR_RGB args_none {
		addIntOp(OP_PUSHINT, ERROR_RGB);
	}
	| B256ERROR_SERIALPARAMETER args_none {
		addIntOp(OP_PUSHINT, ERROR_SERIALPARAMETER);
	}
	| B256ERROR_SETTINGMAXKEYS args_none {
		addIntOp(OP_PUSHINT, ERROR_SETTINGMAXKEYS);
	}
	| B256ERROR_SETTINGMAXLEN args_none {
		addIntOp(OP_PUSHINT, ERROR_SETTINGMAXLEN);
	}
	| B256ERROR_SETTINGSGETACCESS args_none {
		addIntOp(OP_PUSHINT, ERROR_SETTINGSGETACCESS);
	}
	| B256ERROR_SETTINGSSETACCESS args_none {
		addIntOp(OP_PUSHINT, ERROR_SETTINGSSETACCESS);
	}
	| B256ERROR_SLICESIZE args_none {
		addIntOp(OP_PUSHINT, ERROR_SLICESIZE);
	}
	| B256ERROR_SOUNDERROR args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDERROR);
	}
	| B256ERROR_SOUNDFILE args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDFILE);
	}
	| B256ERROR_SOUNDFILEFORMAT args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDFILEFORMAT);
	}
	| B256ERROR_SOUNDLENGTH args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDLENGTH);
	}
	| B256ERROR_SOUNDNOTSEEKABLE args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDNOTSEEKABLE);
	}
	| B256ERROR_SOUNDRESOURCE args_none {
		addIntOp(OP_PUSHINT, ERROR_SOUNDRESOURCE);
	}
	| B256ERROR_SPRITENA args_none {
		addIntOp(OP_PUSHINT, ERROR_SPRITENA);
	}
	| B256ERROR_SPRITENUMBER args_none {
		addIntOp(OP_PUSHINT, ERROR_SPRITENUMBER);
	}
	| B256ERROR_SPRITESLICE args_none {
		addIntOp(OP_PUSHINT, ERROR_SPRITESLICE);
	}
	| B256ERROR_SQRRANGE args_none {
		addIntOp(OP_PUSHINT, ERROR_SQRRANGE);
	}
	| B256ERROR_STACKUNDERFLOW args_none {
		addIntOp(OP_PUSHINT, ERROR_STACKUNDERFLOW);
	}
	| B256ERROR_STRING2NOTE args_none {
		addIntOp(OP_PUSHINT, ERROR_STRING2NOTE);
	}
	| B256ERROR_STRINGCONV args_none {
		addIntOp(OP_PUSHINT, ERROR_STRINGCONV);
	}
	| B256ERROR_STRINGEXPR args_none {
		addIntOp(OP_PUSHINT, ERROR_STRINGEXPR);
	}
	| B256ERROR_STRINGMAXLEN args_none {
		addIntOp(OP_PUSHINT, ERROR_STRINGMAXLEN);
	}
	| B256ERROR_STRSTART args_none {
		addIntOp(OP_PUSHINT, ERROR_STRSTART);
	}
	| B256ERROR_TOOMANYSOUNDS args_none {
		addIntOp(OP_PUSHINT, ERROR_TOOMANYSOUNDS);
	}
	| B256ERROR_UNEXPECTEDRETURN args_none {
		addIntOp(OP_PUSHINT, ERROR_UNEXPECTEDRETURN);
	}
	| B256ERROR_UNSERIALIZEFORMAT args_none {
		addIntOp(OP_PUSHINT, ERROR_UNSERIALIZEFORMAT);
	}
	| B256ERROR_VARCIRCULAR args_none {
		addIntOp(OP_PUSHINT, ERROR_VARCIRCULAR);
	}
	| B256ERROR_VARNOTASSIGNED args_none {
		addIntOp(OP_PUSHINT, ERROR_VARNOTASSIGNED);
	}
	| B256ERROR_VARNULL args_none {
		addIntOp(OP_PUSHINT, ERROR_VARNULL);
	}
	| B256ERROR_WAVEFORMLOGICAL args_none {
		addIntOp(OP_PUSHINT, ERROR_WAVEFORMLOGICAL);
	}
	| B256ERROR_WAVOBSOLETE args_none {
		addIntOp(OP_PUSHINT, ERROR_WAVOBSOLETE);
	}
	| B256WARNING_ARRAYELEMENT args_none {
		addIntOp(OP_PUSHINT, WARNING_ARRAYELEMENT);
	}
	| B256WARNING_BOOLEANCONV args_none {
		addIntOp(OP_PUSHINT, WARNING_BOOLEANCONV);
	}
	| B256WARNING_INTEGERRANGE args_none {
		addIntOp(OP_PUSHINT, WARNING_INTEGERRANGE);
	}
	| B256WARNING_LONGRANGE args_none {
		addIntOp(OP_PUSHINT, WARNING_LONGRANGE);
	}
	| B256WARNING_NUMBERCONV args_none {
		addIntOp(OP_PUSHINT, WARNING_NUMBERCONV);
	}
	| B256WARNING_REFNOTASSIGNED args_none {
		addIntOp(OP_PUSHINT, WARNING_REFNOTASSIGNED);
	}
	| B256WARNING_SOUNDERROR args_none {
		addIntOp(OP_PUSHINT, WARNING_SOUNDERROR);
	}
	| B256WARNING_SOUNDFILEFORMAT args_none {
		addIntOp(OP_PUSHINT, WARNING_SOUNDFILEFORMAT);
	}
	| B256WARNING_SOUNDLENGTH args_none {
		addIntOp(OP_PUSHINT, WARNING_SOUNDLENGTH);
	}
	| B256WARNING_SOUNDNOTSEEKABLE args_none {
		addIntOp(OP_PUSHINT, WARNING_SOUNDNOTSEEKABLE);
	}
	| B256WARNING_START args_none {
		addIntOp(OP_PUSHINT, WARNING_START);
	}
	| B256WARNING_STRING2NOTE args_none {
		addIntOp(OP_PUSHINT, WARNING_STRING2NOTE);
	}
	| B256WARNING_STRINGCONV args_none {
		addIntOp(OP_PUSHINT, WARNING_STRINGCONV);
	}
	| B256WARNING_VARNOTASSIGNED args_none {
		addIntOp(OP_PUSHINT, WARNING_VARNOTASSIGNED);
	}
	| B256WARNING_WAVOBSOLETE args_none {
		addIntOp(OP_PUSHINT, WARNING_WAVOBSOLETE);
	}
	;


/* ###########################################
   ### numeric expressions                 ###
   ########################################### */
expr_numeric:
	B256INTEGER { addIntOp(OP_PUSHINT, $1); }
	| B256FLOAT   {
		if(isfinite($1)){
			addFloatOp(OP_PUSHFLOAT, $1);
		}else{
			errorcode = COMPERR_NUMBERTOOLARGE;
			return -1;
		}
	}

	| '+' B256INTEGER %prec B256UNARY {
		 // accept/eat unary plus only for numbers
		 addIntOp(OP_PUSHINT, $2);
	}

	| '+' B256FLOAT %prec B256UNARY {
		// accept/eat unary plus only for numbers
		if(isfinite($2)){
			addFloatOp(OP_PUSHFLOAT, $2);
		}else{
			errorcode = COMPERR_NUMBERTOOLARGE;
			return -1;
		}
	}			
	| '-' expr %prec B256UNARY {
		addOp(OP_NEGATE);
	}

	| expr '-' expr {
		addOp(OP_SUB);
	}
	| expr B256MOD expr {
		addOp(OP_MOD);
	}
	| expr '%' expr %prec B256MOD {
		addOp(OP_MOD);
	}
	| expr '%' %prec B256UNARY {
		/* expression% is actually a percentage */
		addIntOp(OP_PUSHINT, 100);
		addOp(OP_DIV);
	}
	| expr B256INTDIV expr {
		addOp(OP_INTDIV);
	}
	| expr '/' expr {
		addOp(OP_DIV);
	}
	| expr '^' expr { addOp(OP_EX); }
	| expr B256BINARYOR expr { addOp(OP_BINARYOR); }
	| expr B256BITSHIFTL expr { addOp(OP_BITSHIFTL); }
	| expr B256BITSHIFTR expr { addOp(OP_BITSHIFTR); }
	| B256BINARYNOT expr { addOp(OP_BINARYNOT); }
	| expr B256AND expr {
		addOp(OP_AND);
	}
	| expr B256OR expr {
		addOp(OP_OR);
	}
	| expr B256XOR expr { addOp(OP_XOR); }
	| B256NOT expr %prec B256AND { addOp(OP_NOT); }
	| expr '=' expr { addOp(OP_EQUAL); }
	| expr B256NE expr { addOp(OP_NEQUAL); }
	| expr '<' expr { addOp(OP_LT); }
	| expr '>' expr { addOp(OP_GT); }
	| expr B256GTE expr { addOp(OP_GTE); }
	| expr B256LTE expr { addOp(OP_LTE); }
	| expr B256IN expr {
		addOp(OP_IN);
	}
	
	| array_element B256ADD1 {
		// a[b,c]++ - pushing value before to stack
		int v = varnumber[--nvarnumber];
		addOp(OP_STACKDUP2);			// save indexes
		addIntOp(OP_ARR_GET, v);		// get current value
		addOp(OP_STACKDUP);				// duplicate (1 to save and 1 to increment)
		addOp(OP_STACKSAVE);			// save original
		addIntOp(OP_PUSHINT,1);			// add 1
		addOp(OP_ADD);
		addIntOp(OP_ARR_SET, v);		// assign new value
		addOp(OP_STACKUNSAVE);			// put original value on the stack
	}
	| array_element B256SUB1 {
		// a[b,c]-- - pushing value before to stack
		int v = varnumber[--nvarnumber];
		addOp(OP_STACKDUP2);			// save indexes
		addIntOp(OP_ARR_GET, v);		// get current value
		addOp(OP_STACKDUP);				// duplicate (1 to save and 1 to increment)
		addOp(OP_STACKSAVE);			// save original
		addIntOp(OP_PUSHINT,-1);		// subtract 1
		addOp(OP_ADD);
		addIntOp(OP_ARR_SET, v);		// assign new value
		addOp(OP_STACKUNSAVE);			// put original value on the stack
	}
	| B256ADD1 array_element {
		// ++a[b,c] - pushing value after to stack
		int v = varnumber[--nvarnumber];
		addOp(OP_STACKDUP2);			// save indexes
		addIntOp(OP_ARR_GET, v);		// get current value
		addIntOp(OP_PUSHINT,1);			// add 1
		addOp(OP_ADD);
		addOp(OP_STACKDUP);				// duplicate (1 to set 1 to stack)
		addOp(OP_STACKSAVE);			// save 1 to stack
		addIntOp(OP_ARR_SET, v);		// assign new value
		addOp(OP_STACKUNSAVE);			// put new value on the stack
	}
	| B256SUB1 array_element {
		// --a[b,c] - pushing value after to stack
		int v = varnumber[--nvarnumber];
		addOp(OP_STACKDUP2);			// save indexes
		addIntOp(OP_ARR_GET, v);		// get current value
		addIntOp(OP_PUSHINT,-1);		// subtract 1
		addOp(OP_ADD);
		addOp(OP_STACKDUP);				// duplicate (1 to set 1 to stack)
		addOp(OP_STACKSAVE);			// save 1 to stack
		addIntOp(OP_ARR_SET, v);		// assign new value
		addOp(OP_STACKUNSAVE);			// put new value on the stack
	}
	| variable B256ADD1 {
		addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
		addIntOp(OP_VAR_GET,varnumber[nvarnumber]);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
	}
	| variable B256SUB1 {
		addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
		addIntOp(OP_VAR_GET,varnumber[nvarnumber]);
		addIntOp(OP_PUSHINT,-1);
		addOp(OP_ADD);
		addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
	}
	| B256ADD1 variable {
		addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
		addIntOp(OP_PUSHINT,1);
		addOp(OP_ADD);
		addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
		addIntOp(OP_VAR_GET,varnumber[nvarnumber]);
	}
	| B256SUB1 variable {
		addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
		addIntOp(OP_PUSHINT,-1);
		addOp(OP_ADD);
		addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
		addIntOp(OP_VAR_GET,varnumber[nvarnumber]);
	}
	| B256TOINT '(' expr ')' { addOp(OP_INT); }
	| B256TOFLOAT '(' expr ')' { addOp(OP_FLOAT); }
	| B256LENGTH '(' expr ')' { addOp(OP_LENGTH); }
	| B256ASC '(' expr ')' { addOp(OP_ASC); }
	| B256INSTR '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 1);	// start
		addIntOp(OP_PUSHINT, 0);	// case sens flag
		addOp(OP_INSTR);
	}
	| B256INSTR '(' expr ',' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 0);	// case sens flag
		addOp(OP_INSTR);
	 }
	| B256INSTR '(' expr ',' expr ',' expr ',' expr')' { addOp(OP_INSTR); }
	| B256INSTRX '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 1);	//start
		addOp(OP_INSTRX);
	}
	| B256INSTRX '(' expr ',' expr ',' expr ')' { addOp(OP_INSTRX); }
	| B256CEIL '(' expr ')' { addOp(OP_CEIL); }
	| B256FLOOR '(' expr ')' { addOp(OP_FLOOR); }
	| B256SIN '(' expr ')' { addOp(OP_SIN); }
	| B256COS '(' expr ')' { addOp(OP_COS); }
	| B256TAN '(' expr ')' { addOp(OP_TAN); }
	| B256ASIN '(' expr ')' { addOp(OP_ASIN); }
	| B256ACOS '(' expr ')' { addOp(OP_ACOS); }
	| B256ATAN '(' expr ')' { addOp(OP_ATAN); }
	| B256DEGREES '(' expr ')' { addOp(OP_DEGREES); }
	| B256RADIANS '(' expr ')' { addOp(OP_RADIANS); }
	| B256LOG '(' expr ')' { addOp(OP_LOG); }
	| B256LOGTEN '(' expr ')' { addOp(OP_LOGTEN); }
	| B256SQR '(' expr ')' { addOp(OP_SQR); }
	| B256EXP '(' expr ')' { addOp(OP_EXP); }
	| B256ABS '(' expr ')' { addOp(OP_ABS); }
	| B256RAND args_none { addOp(OP_RAND); }
				| B256PI args_none { addFloatOp(OP_PUSHFLOAT, 3.14159265358979323846); }
	| B256BOOLEOF args_none {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_EOF);
	}
	| B256BOOLEOF '(' expr ')' { addOp(OP_EOF); }
	| B256EXISTS '(' expr ')' { addOp(OP_EXISTS); }
	| B256YEAR args_none { addOp(OP_YEAR); }
	| B256MONTH args_none { addOp(OP_MONTH); }
	| B256DAY args_none { addOp(OP_DAY); }
	| B256HOUR args_none { addOp(OP_HOUR); }
	| B256MINUTE args_none { addOp(OP_MINUTE); }
	| B256SECOND args_none { addOp(OP_SECOND); }
	| B256GRAPHWIDTH args_none { addOp(OP_GRAPHWIDTH); }
	| B256GRAPHHEIGHT args_none { addOp(OP_GRAPHHEIGHT); }
	| B256SIZE args_none {
		addIntOp(OP_PUSHINT, 0);
		addOp(OP_SIZE);
	}
	| B256SIZE '(' expr ')' { addOp(OP_SIZE); }
	| B256KEYPRESSED args_none {
		addIntOp(OP_PUSHINT, 0x00);
		addOp(OP_KEYPRESSED);
	}
	| B256KEYPRESSED '(' expr ')' { addOp(OP_KEYPRESSED); }
	| B256KEY args_none     {
		addIntOp(OP_PUSHINT, 0x00);
		addOp(OP_KEY);
	}
	| B256KEY '(' expr ')'     {
		addOp(OP_KEY);
	}
	| B256MOUSEX args_none { addOp(OP_MOUSEX); }
	| B256MOUSEY args_none { addOp(OP_MOUSEY); }
	| B256MOUSEB args_none { addOp(OP_MOUSEB); }
	| B256CLICKX args_none { addOp(OP_CLICKX); }
	| B256CLICKY args_none { addOp(OP_CLICKY); }
	| B256CLICKB args_none { addOp(OP_CLICKB); }
	| B256PIXEL '(' expr ',' expr ')' { addOp(OP_PIXEL); }
	| B256RGB '(' expr ',' expr ',' expr ')' {
		addIntOp(OP_PUSHINT,255);	// a
		addOp(OP_RGB);
	}
	| B256RGB '(' expr ',' expr ',' expr ',' expr ')' {
		addOp(OP_RGB);
	}
	| B256GETCOLOR args_none { addOp(OP_GETCOLOR); }
	| B256GETBRUSHCOLOR args_none { addOp(OP_GETBRUSHCOLOR); }
	| B256GETPENWIDTH args_none { addOp(OP_GETPENWIDTH); }
	| B256SPRITECOLLIDE '(' expr ',' expr ',' expr ')' { addOp(OP_SPRITECOLLIDE); }
	| B256SPRITECOLLIDE '(' expr ',' expr ')' { addIntOp(OP_PUSHINT, 0); addOp(OP_SPRITECOLLIDE); }
	| B256SPRITEX '(' expr ')' { addOp(OP_SPRITEX); }
	| B256SPRITEY '(' expr ')' { addOp(OP_SPRITEY); }
	| B256SPRITEH '(' expr ')' { addOp(OP_SPRITEH); }
	| B256SPRITEW '(' expr ')' { addOp(OP_SPRITEW); }
	| B256SPRITEV '(' expr ')' { addOp(OP_SPRITEV); }
	| B256SPRITER '(' expr ')' { addOp(OP_SPRITER); }
	| B256SPRITES '(' expr ')' { addOp(OP_SPRITES); }
	| B256SPRITEO '(' expr ')' { addOp(OP_SPRITEO); }
	| B256DBROW args_none {
		addIntOp(OP_PUSHINT,0);	// default db number
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_DBROW);
	}
	| B256DBROW '(' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_DBROW);
	}
	| B256DBROW '(' expr ',' expr')' {
		addOp(OP_DBROW);
	}
	| B256DBINT '(' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBINT); }
	| B256DBINT '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBINT); }
	| B256DBINT '(' expr ',' expr ',' expr ')' {
		addOp(OP_DBINT); }
	| B256DBFLOAT '(' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBFLOAT); }
	| B256DBFLOAT '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBFLOAT); }
	| B256DBFLOAT '(' expr ',' expr ',' expr ')' {
		addOp(OP_DBFLOAT); }
	| B256DBNULL '(' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBNULL); }
	| B256DBNULL '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBNULL); }
	| B256DBNULL '(' expr ',' expr ',' expr ')' {
		addOp(OP_DBNULL); }
	| B256LASTERROR args_none { addOp(OP_LASTERROR); }
	| B256LASTERRORLINE args_none { addOp(OP_LASTERRORLINE); }
	| B256NETDATA args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETDATA); }
	| B256NETDATA '(' expr ')' { addOp(OP_NETDATA); }
	| B256PORTIN '(' expr ')' { addOp(OP_PORTIN); }
	| B256COUNT '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 0); // case sens flag
		addOp(OP_COUNT);
	 }
	| B256COUNT '(' expr ',' expr ',' expr ')' { addOp(OP_COUNT); }
	| B256COUNTX '(' expr ',' expr ')' { addOp(OP_COUNTX); }
	| B256OSTYPE args_none { addOp(OP_OSTYPE); }
	| B256MSEC args_none { addOp(OP_MSEC); }
	| B256TEXTWIDTH '(' expr ')' { addOp(OP_TEXTWIDTH); }
	| B256TEXTWIDTH '(' expr ',' expr ')' { addOp(OP_TEXTBOXWIDTH); }
	| B256TEXTHEIGHT args_none { addOp(OP_TEXTHEIGHT); }
	| B256TEXTHEIGHT '(' expr ',' expr ')' { addOp(OP_TEXTBOXHEIGHT); }
	| B256READBYTE args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READBYTE); }
	| B256READBYTE '(' expr ')' { addOp(OP_READBYTE); }
	| B256FREEDB args_none { addOp(OP_FREEDB); }
	| B256FREEDBSET args_none {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_FREEDBSET);
	}
	| B256FREEDBSET '(' expr ')' { addOp(OP_FREEDBSET); }
	| B256FREEFILE args_none { addOp(OP_FREEFILE); }
	| B256FREENET args_none { addOp(OP_FREENET); }
	| B256VERSION args_none { addIntOp(OP_PUSHINT, VERSIONSIGNATURE); }
	| B256CONFIRM '(' expr ')' {
		addIntOp(OP_PUSHINT,-1);	// no default
		addOp(OP_CONFIRM);
	}
	| B256CONFIRM '(' expr ',' expr ')' {
		addOp(OP_CONFIRM);
	}
	| B256FROMBINARY '(' expr ')' {
		addIntOp(OP_PUSHINT,2);	// radix
		addOp(OP_FROMRADIX);
	}
	| B256FROMHEX '(' expr ')' {
		addIntOp(OP_PUSHINT,16);	// radix
		addOp(OP_FROMRADIX);
	}
	| B256FROMOCTAL '(' expr ')' {
		addIntOp(OP_PUSHINT,8);	// radix
		addOp(OP_FROMRADIX);
	}
	| B256FROMRADIX '(' expr ',' expr ')' {
		addOp(OP_FROMRADIX);
	}
	| B256BINCONST {
		addIntOp(OP_PUSHINT,strtoul($1, NULL, 2));
		if(errno==ERANGE){
			errorcode = COMPERR_NUMBERTOOLARGE;
			return -1;
		}
		//addStringOp(OP_PUSHSTRING, $1);
		//addIntOp(OP_PUSHINT,2);	// radix
		//addOp(OP_FROMRADIX);
	}
	| B256HEXCONST {
		addIntOp(OP_PUSHINT,strtoul($1, NULL, 16));
		if(errno==ERANGE){
			errorcode = COMPERR_NUMBERTOOLARGE;
			return -1;
		}
		//addStringOp(OP_PUSHSTRING, $1);
		//addIntOp(OP_PUSHINT,16);	// radix
		//addOp(OP_FROMRADIX);
	}
	| B256OCTCONST {
		addIntOp(OP_PUSHINT,strtoul($1, NULL, 8));
		if(errno==ERANGE){
			errorcode = COMPERR_NUMBERTOOLARGE;
			return -1;
		}
		//addStringOp(OP_PUSHSTRING, $1);
		//addIntOp(OP_PUSHINT,8);	// radix
		//addOp(OP_FROMRADIX);
	}
	| B256WAVLENGTH args_none { addOp(OP_WAVLENGTH); }
	| B256WAVPOS args_none { addOp(OP_WAVPOS); }
	| B256WAVSTATE args_none { addOp(OP_WAVSTATE); }

	| B256TYPEOF '(' expr ')' {
		addOp(OP_TYPEOF);
	}

	| B256SOUNDPLAYER '(' expr ')' {
		addOp(OP_SOUNDPLAYER);
	}
	| B256SOUNDPLAYER '(' args_ee ')' {
		addIntOp(OP_PUSHINT, 2);	// 2 columns
		addIntOp(OP_PUSHINT, 1);	// 1 row
		addOp(OP_LIST2ARRAY);
		addOp(OP_SOUNDPLAYER);
	}
	| B256SOUNDID args_none {
		addOp(OP_SOUNDID);
	}
	| B256SOUNDPOSITION '(' expr ')' {
		addOp(OP_SOUNDPOSITION);
	}
	| B256SOUNDPOSITION args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDPOSITION);
	}
	| B256SOUNDSTATE '(' expr ')' {
		addOp(OP_SOUNDSTATE);
	}
	| B256SOUNDSTATE args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDSTATE);
	}
	| B256SOUNDLENGTH '(' expr ')' {
		addOp(OP_SOUNDLENGTH);
	}
	| B256SOUNDLENGTH args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDLENGTH);
	}
	| B256SOUNDSAMPLERATE args_none {
		addOp(OP_SOUNDSAMPLERATE);
	}
	| B256IMAGEWIDTH '(' expr ')' {
		addOp(OP_IMAGEWIDTH);
	}
	| B256IMAGEHEIGHT '(' expr ')' {
		addOp(OP_IMAGEHEIGHT);
	}
	| B256IMAGEPIXEL '(' args_eee ')' {
		addOp(OP_IMAGEPIXEL);
	}
	| B256ROUND '(' expr ')' {
		addIntOp(OP_PUSHINT,0);		// default decimal places
		addOp(OP_ROUND);
	}
	| B256ROUND '(' args_ee ')' {
		addOp(OP_ROUND);
	}
	| B256ASSIGNED '(' variable_a ')' {
		addIntOp(OP_VAR_ASSIGNED, varnumber[--nvarnumber]);
	}
	| B256ASSIGNED '(' array_element ')' {
		addIntOp(OP_ARR_ASSIGNED, varnumber[--nvarnumber]);
	}
	| variable array_size {
		addIntOp(OP_ALEN, varnumber[--nvarnumber]);
	}
	| variable array_size_rows {
		addIntOp(OP_ALENROWS, varnumber[--nvarnumber]);
	}
	| variable array_size_cols {
		addIntOp(OP_ALENCOLS, varnumber[--nvarnumber]);
	}
	| B256GETARRAYBASE args_none { addOp(OP_GETARRAYBASE); }
	;
	
/* ###########################################
   ### string expressions                  ###
   ########################################### */

expr_string:
	B256STRING { addStringOp(OP_PUSHSTRING, $1); }
	| B256CHR '(' expr ')' { addOp(OP_CHR); }
	| B256TOSTRING '(' expr ')' { addOp(OP_STRING); }
	| B256UPPER '(' expr ')' { addOp(OP_UPPER); }
	| B256LOWER '(' expr ')' { addOp(OP_LOWER); }
	| B256MID '(' expr ',' expr ',' expr ')' { addOp(OP_MID); }
	| B256MIDX '(' expr ',' expr ')' { addIntOp(OP_PUSHINT, 1); addOp(OP_MIDX); }
	| B256MIDX '(' expr ',' expr ',' expr ')' { addOp(OP_MIDX); }
	| B256LEFT '(' expr ',' expr ')' { addOp(OP_LEFT); }
	| B256RIGHT '(' expr ',' expr ')' { addOp(OP_RIGHT); }
	| B256READ args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READ); }
	| B256READ '(' expr ')' { addOp(OP_READ); }
	| B256READLINE args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_READLINE); }
	| B256READLINE '(' expr ')' { addOp(OP_READLINE); }
	| B256CURRENTDIR args_none { addOp(OP_CURRENTDIR); }
	| B256DBSTRING '(' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default db number
		addOp(OP_STACKSWAP);
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBSTRING); }
	| B256DBSTRING '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT,0);	// default dbset number
		addOp(OP_STACKSWAP);
		addOp(OP_DBSTRING); }
	| B256DBSTRING '(' expr ',' expr ',' expr ')' {
		addOp(OP_DBSTRING); }
	| B256LASTERRORMESSAGE args_none { addOp(OP_LASTERRORMESSAGE); }
	| B256LASTERROREXTRA args_none { addOp(OP_LASTERROREXTRA); }
	| B256NETREAD  args_none { addIntOp(OP_PUSHINT, 0); addOp(OP_NETREAD); }
	| B256NETREAD '(' expr ')' { addOp(OP_NETREAD); }
	| B256NETADDRESS args_none { addOp(OP_NETADDRESS); }
	| B256MD5 '(' expr ')' { addOp(OP_MD5); }
	| B256GETCLIPBOARDIMAGE args_none { addOp(OP_GETCLIPBOARDIMAGE); }
	| B256GETCLIPBOARDSTRING args_none { addOp(OP_GETCLIPBOARDSTRING); }
	| B256GETSETTING '(' expr ',' expr ')' { addOp(OP_GETSETTING); }
	| B256DIR '(' expr ')' { addOp(OP_DIR); }
	| B256DIR args_none { addStringOp(OP_PUSHSTRING, ""); addOp(OP_DIR); }
	| B256REPLACE '(' expr ',' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 0);	// case sens flag
		addOp(OP_REPLACE);
	}
	| B256REPLACE '(' expr ',' expr ',' expr ',' expr ')' { addOp(OP_REPLACE); }
	| B256REPLACEX '(' expr ',' expr ',' expr ')' { addOp(OP_REPLACEX); }
	| B256SERIALIZE '(' expr ')' {
		addOp(OP_SERIALIZE);
	}
	| B256IMPLODE '(' expr ')' {
		addStringOp(OP_PUSHSTRING, ""); // no delimiter
		addOp(OP_STACKDUP);
		addOp(OP_IMPLODE);
	}
	| B256IMPLODE '(' args_ee ')' {
		addOp(OP_STACKDUP);				// same delimiter for rows and columns
		addOp(OP_IMPLODE);
	}
	| B256IMPLODE '(' args_eee ')' {
		addOp(OP_IMPLODE);
	}
	| B256PROMPT '(' expr ')' {
		addStringOp(OP_PUSHSTRING, "");
		addOp(OP_PROMPT); }
	| B256PROMPT '(' expr ',' expr ')' {
		addOp(OP_PROMPT); }
	| B256TOBINARY '(' expr ')' {
		addIntOp(OP_PUSHINT,2);	// radix
		addOp(OP_TORADIX);
	}
	| B256TOHEX '(' expr ')' {
		addIntOp(OP_PUSHINT,16);	// radix
		addOp(OP_TORADIX);
	}
	| B256TOOCTAL '(' expr ')' {
		addIntOp(OP_PUSHINT,8);	// radix
		addOp(OP_TORADIX);
	}
	| B256TORADIX '(' expr ',' expr ')' {
		addOp(OP_TORADIX);
	}
	| B256DEBUGINFO '(' expr ')' {
		addOp(OP_DEBUGINFO);
	}
	| B256ISNUMERIC '(' expr ')' { addOp(OP_ISNUMERIC); }
	| B256LTRIM '(' expr ')' { addOp(OP_LTRIM); }
	| B256RTRIM '(' expr ')' { addOp(OP_RTRIM); }
	| B256TRIM '(' expr ')' { addOp(OP_TRIM); }
	| B256IMAGETYPE_BMP args_none { addStringOp(OP_PUSHSTRING, IMAGETYPE_BMP); }
	| B256IMAGETYPE_JPG args_none { addStringOp(OP_PUSHSTRING, IMAGETYPE_JPG); }
	| B256IMAGETYPE_PNG args_none { addStringOp(OP_PUSHSTRING, IMAGETYPE_PNG); }
	| B256SOUNDLOAD '(' expr ')' {
		addOp(OP_SOUNDLOAD);
	}
	| B256SOUNDLOAD '(' args_ee ')' {
		addIntOp(OP_PUSHINT, 2);	// 2 columns
		addIntOp(OP_PUSHINT, 1);	// 1 row
		addOp(OP_LIST2ARRAY);
		addOp(OP_SOUNDLOAD);
	}
	| B256SOUNDLOADRAW '(' expr ')' {
		addOp(OP_SOUNDLOADRAW);
	}
	| B256IMAGENEW '(' expr ',' expr ',' expr ')' {
		addOp(OP_IMAGENEW);
	}
	| B256IMAGENEW '(' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 0x00);
		addOp(OP_IMAGENEW);
	}
	| B256IMAGELOAD '(' expr ')' {
		addOp(OP_IMAGELOAD);
	}
	| B256IMAGECOPY '(' expr ',' expr ',' expr ',' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 5); //number of arguments
		addOp(OP_IMAGECOPY);
	}
	| B256IMAGECOPY '(' expr ',' expr ',' expr ',' expr ')' {
		addIntOp(OP_PUSHINT, 4); //number of arguments
		addOp(OP_IMAGECOPY);
	}
	| B256IMAGECOPY '(' expr ')' {
		addIntOp(OP_PUSHINT, 1); //number of arguments
		addOp(OP_IMAGECOPY);
	}
	| B256IMAGECOPY args_none {
		addIntOp(OP_PUSHINT, 0); //number of arguments
		addOp(OP_IMAGECOPY);
	}
	| B256LJUST '(' args_ee ')' {
		addStringOp(OP_PUSHSTRING, " ");
		addOp(OP_LJUST);
	}
	| B256LJUST '(' args_eee ')' {
		addOp(OP_LJUST);
	}
	| B256RJUST '(' args_ee ')' {
		addStringOp(OP_PUSHSTRING, " ");
		addOp(OP_RJUST);
	}
	| B256RJUST '(' args_eee ')' {
		addOp(OP_RJUST);
	}
	| B256ZFILL '(' args_ee ')' {
		addStringOp(OP_PUSHSTRING, "0");
		addOp(OP_RJUST);
	}
	| B256OPENFILEDIALOG '(' args_eee ')' {
		addOp(OP_OPENFILEDIALOG);
	}
	| B256SAVEFILEDIALOG '(' args_eee ')' {
		addOp(OP_SAVEFILEDIALOG);
	}
	;



/* ###############################################################
   ### DataElement object                                      ###
   ### these may be a variable's associated DataElement object ###
   ### or a DataElement object containing an array of data     ###
   ############################################################### */

expr_dataelement:
	variable_a {
		addIntOp(OP_VAR_GET, varnumber[--nvarnumber]);
	}
	| listoflists {
		addOp(OP_LIST2ARRAY);
	}
	| listofmapitems {
		addOp(OP_LIST2MAP);
	}
	| B256UNSERIALIZE '(' expr ')'{
		addOp(OP_UNSERIALIZE);
	}
	
	| B256EXPLODE args_ee {
		addIntOp(OP_PUSHINT, 0);	// case sensitive flag
		addOp(OP_EXPLODE);
	}
	| B256EXPLODE args_eee {
		addOp(OP_EXPLODE);
	}
	|  B256EXPLODEX args_ee{
		addOp(OP_EXPLODEX);
	}

	| B256GETSLICE args_eeee {
		addIntOp(OP_PUSHINT, SLICE_ALL);	// get everything
		addOp(OP_GETSLICE);
	}
	| B256GETSLICE args_eeeee {
		addOp(OP_GETSLICE);
	}
	;




/* now the list of statements */

statement:
	alertstmt
	| arcstmt
	| arraybasestmt
	| begincasestmt
	| callstmt
	| casestmt
	| catchstmt
	| changedirstmt
	| chordstmt
	| circlestmt
	| clearstmt
	| clickclearstmt
	| closestmt
	| colorstmt
	| continuedostmt
	| continueforstmt
	| continuewhilestmt
	| dbclosesetstmt
	| dbclosestmt
	| dbexecutestmt
	| dbopensetstmt
	| dbopenstmt
	| dimstmt
	| dostmt
	| setgraphstmt
	| editvisiblestmt
	| ellipsestmt
	| elsestmt
	| endcasestmt
	| endfunctionstmt
	| endifstmt
	| endstmt
	| endsubroutinestmt
	| endtrystmt
	| endwhilestmt
	| exitdostmt
	| exitforstmt
	| exitwhilestmt
	| fastgraphicsstmt
	| fontstmt
	| forstmt
	| foreachstmt
	| functionstmt
	| globalstmt
	| gosubstmt
	| gotostmt
	| graphsizestmt
	| graphvisiblestmt
	| graphtoolbarvisiblestmt
	| ifstmt
	| ifthenstmt
	| ifthenelsestmt
	| imageautocropstmt
	| imagecenteredstmt
	| imagecropstmt
	| imagedrawstmt
	| imageflipstmt
	| imagesetpixelstmt
	| imageresizestmt
	| imagerotatestmt
	| imagesmoothstmt
	| imagetransformedstmt
	| imgloadstmt
	| imgsavestmt
	| inputstmt
	| killstmt
	| letstmt
	| linestmt
	| maintoolbarvisiblestmt
	| mapstmt
	| netclosestmt
	| netconnectstmt
	| netlistenstmt
	| netwritestmt
	| nextstmt
	| offerrorstmt
	| onerrorstmt
	| openstmt
	| outputvisiblestmt
	| outputtoolbarvisiblestmt
	| pausestmt
	| penwidthstmt
	| piestmt
	| plotstmt
	| polystmt
	| portoutstmt
	| printercancelstmt
	| printeroffstmt
	| printeronstmt
	| printerpagestmt
	| printstmt
	| putslicestmt
	| rectstmt
	| redimstmt
	| refreshstmt
	| regexminimalstmt
	| resetstmt
	| returnstmt
	| saystmt
	| seedstmt
	| seekstmt
	| setclipboardimagestmt
	| setclipboardstringstmt
	| setsettingstmt
	| soundstmt
	| soundpausestmt
	| soundplayeroffstmt
	| soundplaystmt
	| soundstopstmt
	| soundwaitstmt
	| soundwaveformstmt
	| soundsystemstmt
	| soundenvelopestmt
	| soundharmonicsstmt
	| soundfadestmt
	| soundseekstmt
	| soundvolumestmt
	| soundloopstmt
	| spritedimstmt
	| spritehidestmt
	| spriteloadstmt
	| spritemovestmt
	| spriteplacestmt
	| spritepolystmt
	| spriteshowstmt
	| spriteslicestmt
	| stampstmt
	| subroutinestmt
	| systemstmt
	| textstmt
	| throwerrorstmt
	| trystmt
	| unassignstmt
	| unloadstmt
	| untilstmt
	| variablewatchstmt
	| volumestmt
	| wavpausestmt
	| wavplaystmt
	| wavseekstmt
	| wavstopstmt
	| wavwaitstmt
	| whilestmt
	| writebytestmt
	| writelinestmt
	| writestmt
	;


begincasestmt:
	B256BEGINCASE {
		// start a case block
		newIf(linenumber, IFTABLETYPEBEGINCASE,-1);
	}
	;


casestmt:	B256CASE {
		// if not first case then add jump to to "endcase" and resolve the branchfalse
		if (numifs>1) {
			if (iftabletype[numifs-1]==IFTABLETYPECASE) {
				if (iftabletype[numifs-2]==IFTABLETYPEBEGINCASE) {
					//
					// create jump around from end of the CASE to end of the END CASE
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-2],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_ENDBEGINCASE;
					linenumber = iftablesourceline[numifs-1];
					return -1;
				}
				//
				// resolve branchfalse from previous case
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				//
				numifs--;
			}
		}
		//
	} expr {
		//
		// add branch to the end if false
		addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
		//
		// put new CASE on the frame for the IF
		newIf(linenumber, IFTABLETYPECASE,-1);
	}
	;

catchstmt: 	B256CATCH {
	//
	// create jump around from end of the TRY to end of the CATCH
	// OP_OFFERRORCATCH label - close try/catch trap and jump over the CATCH part
	addIntOp(OP_OFFERRORCATCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
	//
	if (numifs>0) {
		if (iftabletype[numifs-1]==IFTABLETYPETRY) {
			//
			// resolve the try onerrorcatch to the catch address
			symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
			numifs--;
			//
			// put new if on the frame for the catch
			newIf(linenumber, IFTABLETYPECATCH,-1);
		} else {
			errorcode = testIfOnTableError(numincludes);
			linenumber = testIfOnTable(numincludes);
			return -1;
		}
	} else {
		errorcode = COMPERR_CATCH;
		return -1;
	}
}
;

dostmt: 	B256DO {
		//
		// create internal symbol and add to the label table for the top of the loop
		symtableaddress[getInternalSymbol(nextifid,INTERNALSYMBOLTOP)] = wordOffset;
		//
		// add to if frame
		newIf(linenumber, IFTABLETYPEDO, -1);
	}
	;

elsestmt:	B256ELSE {
		//
		// create jump around from end of the THEN to end of the ELSE
		addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
		//
		if (numifs>0) {
			if (iftabletype[numifs-1]==IFTABLETYPEIF) {
				//
				// resolve the label on the if to the current location
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
				//
				// put new if on the frame for the else
				newIf(linenumber, IFTABLETYPEELSE, -1);
			} else if (iftabletype[numifs-1]==IFTABLETYPECASE) {
				if (numifs>1) {
					if (iftabletype[numifs-2]==IFTABLETYPEBEGINCASE) {
						//
						// create jump around from end of the CASE to end of the END CASE
						addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-2],INTERNALSYMBOLEXIT));
					} else {
						errorcode = COMPERR_ENDBEGINCASE;
						linenumber = iftablesourceline[numifs-1];
						return -1;
					}
					//
					// resolve branchfalse from previous case
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					//
					numifs--;
					// put new if on the frame for the else
					newIf(linenumber, IFTABLETYPEELSE, -1);
				}
			} else {
				errorcode = testIfOnTableError(numincludes);
				linenumber = testIfOnTable(numincludes);
				return -1;
			}
		} else {
			errorcode = COMPERR_ELSE;
			return -1;
		}
	}
	;

endcasestmt:
			B256ENDCASE {
				// add label for last case branchfalse to jump to
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPECASE || iftabletype[numifs-1]==IFTABLETYPEELSE) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = COMPERR_ENDENDCASE;
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDENDCASE;
					return -1;
				}
				// add label for all cases to jump to after execution
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEBEGINCASE) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDENDCASEBEGIN;
					return -1;
				}
			}
			;

endifstmt:	B256ENDIF {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then put the current wordcode address there
				// - so we can jump over code
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEIF||iftabletype[numifs-1]==IFTABLETYPEELSE) {
						//
						// resolve the label on the if/else to the current location
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDIF;
					return -1;
				}
			}
			;

endtrystmt:	B256ENDTRY {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then put the current wordcode address there
				// - so we can jump over code
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPECATCH) {
						//
						// resolve the label on the Catch to the current location
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDTRY;
					return -1;
				}
			}
			;

endwhilestmt:
			B256ENDWHILE {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEWHILE) {
						//
						// jump to the top
						addIntOp(OP_GOTO, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE));
						//
						// resolve the label to the bottom
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						//
						// remove the single placeholder from the if frame
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_ENDWHILE;
					return -1;
				}
			}
			;

ifstmt:		B256IF expr B256THEN {
					//
					// add branch to the end if false
					addIntOp(OP_BRANCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
					//
					// put new if on the frame for the IF
					newIf(linenumber, IFTABLETYPEIF, -1);
			}
			;

ifthenstmt:
			ifstmt compoundstmt {
				// if there is an if branch or jump on the iftable stack get where it is
				// in the wordcode array and then resolve the lable
				if (numifs>0) {
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					numifs--;
				}
			}
			;

ifthenelsestmt:
			ifthenelse compoundstmt {
				//
				// resolve the label on the else to the current location
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
			}
			;

ifthenelse:
			ifstmt compoundstmt B256ELSE {
				//
				// create jump around from end of the THEN to end of the ELSE
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// jump point for else
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
				//
				// put new if on the frame for the else
				newIf(linenumber, IFTABLETYPEELSE, -1);
			}
			;

trystmt: 	B256TRY	{
				//
				// add on error branch
                                addIntOp(OP_ONERRORCATCH, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// put new if on the frame for the TRY
				newIf(linenumber, IFTABLETYPETRY, -1);
			}
			;


untilstmt:	B256UNTIL {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEDO) {
						//
						// create label for CONTINUE DO
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
						//
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_UNTIL;
					return -1;
				}
			} expr {
				//
				// branch back to top if condition holds
				addIntOp(OP_BRANCH, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLTOP));
				//
				// create label for EXIT DO
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
				numifs--;
			}
			;

whilestmt: 		B256WHILE {
				//
				// create internal symbol and add to the label table for the top of the loop
				newIf(linenumber, IFTABLETYPEWHILE, -1);
				symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
			} expr {
				//
				// add branch to end if false
				addIntOp(OP_BRANCH, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
			};

letstmt:	B256LET assign
			| B256LET arrayassign
			| B256LET arrayelementassign
			| assign
			| arrayassign
			| arrayelementassign
			;

mapstmt: 	B256MAP functionvariables {
				for(int t=numargs-1;t>=0;t--) {
					addIntOp(OP_MAP_DIM, args[t]);
				}
				numargs=0;	// clear the list for next function
			}

dimstmt: 	B256DIM array_element {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM array_element B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM variable_a expr {
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM variable_a expr B256FILL expr {
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM variable_a args_ee {
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
			}
			| B256DIM variable_a args_ee B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_DIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256DIM variable_a '=' expr {
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256DIM variable_a B256FILL expr {
				addIntOp(OP_PUSHINT, 1);
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			;

redimstmt:	B256REDIM array_element {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM array_element B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256REDIM variable_a expr {
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM variable_a expr B256FILL expr {
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1);
				addOp(OP_STACKSWAP);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			| B256REDIM variable_a args_ee {
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
			}
			| B256REDIM variable_a args_ee B256FILL expr {
				addOp(OP_STACKTOPTO2);
				addIntOp(OP_REDIM, varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT, 0);		// just fill unassigned
				addIntOp(OP_ARRAYFILL, varnumber[nvarnumber]);
			}
			;

pausestmt:	B256PAUSE expr {
				addOp(OP_PAUSE);
			}
			;

throwerrorstmt:
			B256THROWERROR expr {
				addOp(OP_THROWERROR);
			}
			;

clearstmt:	B256CLS args_none {
				addOp(OP_CLS);
			}
			| B256CLG args_none {
				// push the color clear if there are no arguments
				addIntOp(OP_PUSHINT, 0x00);
				//addIntOp(OP_PUSHINT, 0x00);
				//addIntOp(OP_PUSHINT, 0x00);
				//addIntOp(OP_PUSHINT, 0x00);
				//addIntOp(OP_PUSHINT, 0x00);
				//addOp(OP_RGB);
				addOp(OP_CLG);
			}
			| B256CLG expr {
				addOp(OP_CLG);
			}
			;

fastgraphicsstmt:
			B256FASTGRAPHICS args_none {
				addOp(OP_FASTGRAPHICS);
			}
			;

graphsizestmt:
			B256GRAPHSIZE args_ee {
                                addFloatOp(OP_PUSHFLOAT, 1.0);
				addOp(OP_GRAPHSIZE);
			}
			| B256GRAPHSIZE args_eee {
					addOp(OP_GRAPHSIZE);
			}
			;

refreshstmt:
			B256REFRESH args_none {
				addOp(OP_REFRESH);
			}
			;

endstmt: 	B256END args_none {
				addOp(OP_END);
			}
			;

/* assign an expression to a single array element */
arrayelementassign:
			array_element '=' expr {
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| array_element B256ADD1 {
				// a[r,c]++ - NO PUSH as this is a statement
				int v = varnumber[--nvarnumber];
				addOp(OP_STACKDUP2);			// save indexes
				addIntOp(OP_ARR_GET, v);		// get current value
				addIntOp(OP_PUSHINT,1);			// add 1
				addOp(OP_ADD);
				addIntOp(OP_ARR_SET, v);		// assign new value
			}
			| array_element B256SUB1 {
				// a[r,c]-- - NO PUSH as this is a statement
				int v = varnumber[--nvarnumber];
				addOp(OP_STACKDUP2);			// save indexes
				addIntOp(OP_ARR_GET, v);		// get current value
				addIntOp(OP_PUSHINT,-1);		// subtract 1
				addOp(OP_ADD);
				addIntOp(OP_ARR_SET, v);		// assign new value
			}
			| array_element B256ADDEQUAL expr {
				// a[b,c] += n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_ADD);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			| array_element B256SUBEQUAL expr {
				// a[b,c] -= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_SUB);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			| array_element B256MULEQUAL expr {
				// a[b,c] *= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_MUL);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			| array_element B256DIVEQUAL expr {
				// a[b,c] /= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_DIV);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			| array_element B256AMPEQUAL expr {
				// a[b,c] &= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_BINARYAND);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			| array_element B256SEMICOLONEQUAL expr {
				// a[b,c] /= n
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKDUP2);
				addIntOp(OP_ARR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKTOPTO2);
				addOp(OP_STACKSWAP2);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCATENATE);
				addIntOp(OP_ARR_SET, varnumber[nvarnumber]);
			}
			;
			
/* assign an entire array in one statement */
arrayassign:
			variable_a B256FILL expr {
				addIntOp(OP_PUSHINT, 1);		// fill all elements
				addIntOp(OP_ARRAYFILL, varnumber[--nvarnumber]);
			}
			;

/* assign an expression to a normal variable */
assign:
			variable_a '=' expr {
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| variable B256ADD1 {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,1);
				addOp(OP_ADD);
				addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
			}
			| variable B256SUB1 {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addIntOp(OP_PUSHINT,-1);
				addOp(OP_ADD);
				addIntOp(OP_VAR_SET,varnumber[nvarnumber]);
			}
			| variable B256ADDEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_ADD);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			| variable B256SUBEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_SUB);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			| variable B256MULEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_MUL);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			| variable B256DIVEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_DIV);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			| variable B256AMPEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_BINARYAND);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			| variable B256SEMICOLONEQUAL expr {
				addIntOp(OP_VAR_GET,varnumber[--nvarnumber]);
				addOp(OP_STACKSWAP);
				addOp(OP_CONCATENATE);
				addIntOp(OP_VAR_SET, varnumber[nvarnumber]);
			}
			;


forstmt: 	B256FOR variable '=' expr B256TO expr {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				int var =  varnumber[--nvarnumber];
				newIf(linenumber, IFTABLETYPEFOR, var);
				// push default step 1 and exit address
				addIntOp(OP_PUSHINT, 1); //step
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntOp(OP_FOR, var);
			}
			| B256FOR variable '=' expr B256TO expr B256STEP expr {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				int var =  varnumber[--nvarnumber];
				newIf(linenumber, IFTABLETYPEFOR, var);
				// push exit address
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntOp(OP_FOR, var);
			}
			;
			
foreachstmt:	B256FOREACH variable B256IN expr_dataelement {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				int var =  varnumber[--nvarnumber];
				newIf(linenumber, IFTABLETYPEFOR, var);
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntIntOp(OP_FOREACH, var, -1 );
			}
			| B256FOREACH variable '-' '>' variable B256IN expr_dataelement {
				// add to iftable to make sure it is not broken with an if
				// do, while, else, and to report if it is
				// next ed before end of program
				int value_var =  varnumber[--nvarnumber];
				int key_var =  varnumber[--nvarnumber];
				newIf(linenumber, IFTABLETYPEFOR, key_var);
				addIntOp(OP_PUSHLABEL, getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT));
				addIntIntOp(OP_FOREACH, key_var, value_var);
			}
			;

nextstmt:	B256NEXT variable {
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEFOR) {
						if (iftablevariable[numifs-1]==varnumber[nvarnumber-1]) {
							symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
							addOp(OP_NEXT);
							symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
							numifs--;
						} else {
							errorcode = COMPERR_NEXTWRONGFOR;
							return -1;
						}
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_NEXT;
					return -1;
				}
			}
			| B256NEXT {
				// allow relaxed next without the variable name
				if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPEFOR) {
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLCONTINUE)] = wordOffset;
						addOp(OP_NEXT);
						symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
						numifs--;
					} else {
						errorcode = testIfOnTableError(numincludes);
						linenumber = testIfOnTable(numincludes);
						return -1;
					}
				} else {
					errorcode = COMPERR_NEXT;
					return -1;
				}
			}
			;

gotostmt:	B256GOTO variable {
				if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
					errorcode = COMPERR_FUNCTIONGOTO;
					return -1;
				}
				addIntOp(OP_GOTO, varnumber[--nvarnumber]);
			}
			;

gosubstmt:	B256GOSUB variable {
				if (functionDefSymbol != -1 || subroutineDefSymbol !=-1) {
					errorcode = COMPERR_FUNCTIONGOTO;
					return -1;
				}
				addIntOp(OP_GOSUB, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
			}
			;

callstmt:	B256CALL variable '(' ')' {
				addIntOp(OP_PUSHINT, 0); //push number of arguments passed to compare with SUBROUTINE definition
				addIntOp(OP_CALLSUBROUTINE, varnumber[--nvarnumber]);
				addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
			}
			| B256CALL variable '(' callexprlist ')' {
					addIntOp(OP_PUSHINT, listlen); //push number of arguments passed to compare with SUBROUTINE definition
					addIntOp(OP_CALLSUBROUTINE, varnumber[--nvarnumber]);
					addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);
			}
			;

offerrorstmt:
			B256OFFERROR args_none {
				addOp(OP_OFFERROR);
			}
			;

onerrorstmt:
			B256ONERROR variable {
				addIntOp(OP_ONERRORGOSUB, varnumber[--nvarnumber]);
			}
			| B256ONERROR variable '(' ')' {
				addIntOp(OP_ONERRORCALL, varnumber[--nvarnumber]);
			}
			| B256ONERROR variable '(' callexprlist ')' {
				errorcode = COMPERR_ONERRORCALL;
				return -1;
			}
			;

returnstmt:	B256RETURN args_none {
				if (functionDefSymbol!=-1) {
					// if we are defining a function return pushes a variable value
					addIntOp(OP_VAR_GET, functionDefSymbol);
					addOp(OP_DECREASERECURSE);
				}
				if (subroutineDefSymbol!=-1) {
					// if we are defining a subroutine
					addOp(OP_DECREASERECURSE);
				}
				addOp(OP_RETURN);
			}
			| B256RETURN expr {
				if (functionDefSymbol!=-1) {
					// value on stack gets returned
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
				} else {
					errorcode = COMPERR_RETURNVALUE;
					return -1;
				}
			}
			;

colorstmt:	B256SETCOLOR expr {
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
			}
			| B256SETCOLOR args_ee  {
				addOp(OP_SETCOLOR);
			}
			| B256SETCOLOR args_eee {
				addIntOp(OP_PUSHINT, 255);
				addOp(OP_RGB);
				addOp(OP_STACKDUP);
				addOp(OP_SETCOLOR);
				newParseWarning(COMPWARNING_DEPRECATED_FORM);
			}
			;

soundstmt:	B256SOUND args_ee {
				addIntOp(OP_PUSHINT, 2);	// 2 columns (this)
				addIntOp(OP_PUSHINT, 1);	// 1 row
				addIntOp(OP_PUSHINT, 2);	// 2 columns (max)
				addOp(OP_LIST2ARRAY);
				addOp(OP_SOUND);
			}
			| B256SOUND expr {
				addOp(OP_SOUND);
			}
			;

soundplaystmt:	B256SOUNDPLAY args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDPLAY);
	}
	| B256SOUNDPLAY args_ee {
		addIntOp(OP_PUSHINT, 2);	// 2 columns
		addIntOp(OP_PUSHINT, 1);	// 1 row
		addOp(OP_LIST2ARRAY);
		addOp(OP_SOUNDPLAY);
	}
	| B256SOUNDPLAY expr {
		// either string - lodresource
		// int - player number
		// array - from a variable
		addOp(OP_SOUNDPLAY);
	}
	;

soundpausestmt:  B256SOUNDPAUSE expr {
		addOp(OP_SOUNDPAUSE);
	}
	| B256SOUNDPAUSE args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDPAUSE);
	}
	;

soundplayeroffstmt:  B256SOUNDPLAYEROFF expr {
		addOp(OP_SOUNDPLAYEROFF);
	}
	| B256SOUNDPLAYEROFF args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDPLAYEROFF);
	}
	;

soundstopstmt:  B256SOUNDSTOP expr {
		addOp(OP_SOUNDSTOP);
	}
	| B256SOUNDSTOP args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDSTOP);
	}
	;

soundwaitstmt:
	B256SOUNDWAIT expr {
		addOp(OP_SOUNDWAIT);
	}
	| B256SOUNDWAIT args_none {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_SOUNDWAIT);
	}
	;

soundwaveformstmt:  
	B256SOUNDWAVEFORM expr {
		addIntOp(OP_PUSHINT,0);
		addOp(OP_SOUNDWAVEFORM);
	}
	| B256SOUNDWAVEFORM args_ee {
		addOp(OP_SOUNDWAVEFORM);
	}
	;

soundsystemstmt:  B256SOUNDSYSTEM expr {
		addOp(OP_SOUNDSYSTEM);
	}
	;

soundenvelopestmt:  B256SOUNDENVELOPE args_none {
		addOp(OP_SOUNDNOENVELOPE);
	}
	| B256SOUNDENVELOPE args_eeee {
		addOp(OP_SOUNDENVELOPE);
	}
	| B256SOUNDENVELOPE expr {
		addOp(OP_SOUNDENVELOPE);
	}
	;

soundharmonicsstmt:  B256SOUNDHARMONICS args_none {
		addOp(OP_SOUNDNOHARMONICS);
	}
	| B256SOUNDHARMONICS args_ee {
		addOp(OP_SOUNDHARMONICS);
	}
	| B256SOUNDHARMONICS expr {
		addOp(OP_SOUNDHARMONICS);
	}
	;


soundfadestmt:  B256SOUNDFADE args_eeee {
		addOp(OP_SOUNDFADE);
	}
	| B256SOUNDFADE args_eee {
		addOp(OP_STACKTOPTO2);
		addOp(OP_STACKTOPTO2);
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_STACKSWAP);
		addOp(OP_STACKSWAP2);
		addOp(OP_SOUNDFADE);
	}
	;

soundseekstmt:  B256SOUNDSEEK args_ee {
		addOp(OP_SOUNDSEEK);
	}
	| B256SOUNDSEEK expr {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_STACKSWAP);
		addOp(OP_SOUNDSEEK);
	}
	;

soundvolumestmt:  B256SOUNDVOLUME args_ee {
		addOp(OP_SOUNDVOLUME);
	}
	| B256SOUNDVOLUME expr {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_STACKSWAP);
		addOp(OP_SOUNDVOLUME);
	}
	;

soundloopstmt:  B256SOUNDLOOP args_ee {
		addOp(OP_SOUNDLOOP);
	}
	| B256SOUNDLOOP expr {
		addIntOp(OP_PUSHINT, -1);
		addOp(OP_STACKSWAP);
		addOp(OP_SOUNDLOOP);
	}
	;

plotstmt: 	B256PLOT args_ee {
				addOp(OP_PLOT);
			}
			;

linestmt:	B256LINE args_eeee {
				addOp(OP_LINE);
			}
			;


circlestmt:
	B256CIRCLE args_eee {
		addOp(OP_CIRCLE);
	}
	;

ellipsestmt:
	B256ELLIPSE args_eeee {
		addOp(OP_ELLIPSE);
	}
	;

arcstmt:
	B256ARC args_eeeee {
		addIntOp(OP_PUSHINT, 5); // with bounding circle
		addOp(OP_ARC);
	}
	| B256ARC args_eeeeee {
		addIntOp(OP_PUSHINT, 6); // with bounding rectangle
		addOp(OP_ARC);
	}
	;

chordstmt:
	B256CHORD args_eeeee {
		addIntOp(OP_PUSHINT, 5); // with bounding circle
		addOp(OP_CHORD);
	}
	| B256CHORD args_eeeeee {
		addIntOp(OP_PUSHINT, 6); // with bounding rectangle
		addOp(OP_CHORD);
	}
	;

piestmt:
	B256PIE args_eeeee {
		addIntOp(OP_PUSHINT, 5); // with bounding circle
		addOp(OP_PIE);
	}
	| B256PIE args_eeeeee {
		addIntOp(OP_PUSHINT, 6); // with bounding rectangle
		addOp(OP_PIE);
	}
	;

rectstmt:
	B256RECT args_eeee {
		addOp(OP_RECT);
	}
	| B256RECT args_eeeee {
		addOp(OP_STACKDUP);
		addOp(OP_ROUNDEDRECT);
	}
	| B256RECT args_eeeeee {
		addOp(OP_ROUNDEDRECT);
	}
	;

textstmt:
	B256TEXT args_eee {
		addOp(OP_TEXT);
	}
	| B256TEXT args_eeeee {
		addIntOp(OP_PUSHINT, 0); // flags
		addOp(OP_TEXTBOX);
	}
	| B256TEXT args_eeeeee {
		addOp(OP_TEXTBOX);
	}
			;

fontstmt:
	B256FONT args_eeee {
		addOp(OP_FONT);
	}
	| B256FONT args_eee {
		addIntOp(OP_PUSHINT, 0); // font is not italic
		addOp(OP_FONT);
	}
	| B256FONT args_ee {
		addIntOp(OP_PUSHINT, -1); // default weight
		addIntOp(OP_PUSHINT, 0); // font is not italic
		addOp(OP_FONT);
	}
	| B256FONT expr {
		addIntOp(OP_PUSHINT, -1); // default size
		addIntOp(OP_PUSHINT, -1); // default weight
		addIntOp(OP_PUSHINT, 0); // font is not italic
		addOp(OP_FONT);
	}
	;

saystmt: 	B256SAY expr {
				addOp(OP_SAY);
			}
			;

systemstmt:
			B256SYSTEM expr {
				addOp(OP_SYSTEM);
			}
			;

volumestmt:
			B256VOLUME expr {
				addOp(OP_VOLUME);
			}
			;

polystmt:
			B256POLY expr {
				addOp(OP_POLY);
			}
			;

stampstmt: 	B256STAMP args_eeeee {
				addOp(OP_STAMP);
			}
			| B256STAMP args_eeee {
				addFloatOp(OP_PUSHFLOAT, 0.0); // default rotate
				addOp(OP_STACKSWAP);
				addOp(OP_STAMP);
			}
			| B256STAMP args_eee {
				addFloatOp(OP_PUSHFLOAT, 1.0); // default scale
				addOp(OP_STACKSWAP);
				addFloatOp(OP_PUSHFLOAT, 0.0); // default rotate
				addOp(OP_STACKSWAP);
				addOp(OP_STAMP);
			}
			;

openstmt:	B256OPEN expr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			}
			| B256OPEN args_ee {
				addIntOp(OP_PUSHINT, 0); // not binary
				addOp(OP_OPEN);
			}
			| B256OPENB expr  {
				addIntOp(OP_PUSHINT, 0); // file number zero
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			}
			| B256OPENB args_ee {
				addIntOp(OP_PUSHINT, 1); // binary
				addOp(OP_OPEN);
			}
			| B256OPENSERIAL args_ee {
				addIntOp(OP_PUSHINT, 9600); // baud
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eee {
				addIntOp(OP_PUSHINT, 8); // data bits
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeee {
				addIntOp(OP_PUSHINT, 1); // stop bits
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeee {
				addIntOp(OP_PUSHINT, 0); // parity
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeeee {
				addIntOp(OP_PUSHINT, 0); // flow
				addOp(OP_OPENSERIAL);
			}
			| B256OPENSERIAL args_eeeeeee {
				addOp(OP_OPENSERIAL);
			}
			;

writestmt:	B256WRITE expr {
				addIntOp(OP_PUSHINT, 0);  // file number zero
				addOp(OP_STACKSWAP);
				addOp(OP_WRITE);
			}
			| B256WRITE args_ee {
				addOp(OP_WRITE);
			}
			;

writelinestmt:
			B256WRITELINE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITELINE);
			}
			| B256WRITELINE args_ee {
				addOp(OP_WRITELINE);
			}
			;

writebytestmt:
			B256WRITEBYTE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_WRITEBYTE);
			}
			| B256WRITEBYTE args_ee {
				addOp(OP_WRITEBYTE);
			}
			;

closestmt:	B256CLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_CLOSE);
			}
			| B256CLOSE expr {
				addOp(OP_CLOSE);
			}
			;

resetstmt:	B256RESET args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_RESET);
			}
			| B256RESET expr {
				addOp(OP_RESET);
			}
			;

seedstmt:	B256SEED expr {
				addOp(OP_SEED);
			}
			;
			
seekstmt:	B256SEEK expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_SEEK);
			}
			| B256SEEK args_ee {
				addOp(OP_SEEK);
			}
			;

inputstmt:	B256INPUT args_ev {
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUT variable  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUT array_element {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_UNASSIGNED);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_ev {
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING variable  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTSTRING array_element {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_STRING);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_ev {
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTINT variable  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTINT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTINT array_element {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_INT);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_ev {
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT variable  {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_VAR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT args_ea {
				addOp(OP_STACKTOPTO2); addOp(OP_STACKTOPTO2);		// bring prompt to top
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			| B256INPUTFLOAT array_element {
				addStringOp(OP_PUSHSTRING, "");
				addIntOp(OP_PUSHINT,T_FLOAT);
				addOp(OP_INPUT);
				addIntOp(OP_ARR_SET, varnumber[--nvarnumber]);
			}
			;

printstmt:
			B256PRINT args_none {
				addIntOp(OP_PUSHINT, 0); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}
			| B256PRINT expr B256SEMICOLON {
				addIntOp(OP_PUSHINT, 1); //push number of arguments passed
				addIntOp(OP_PUSHINT, 0); // suppress NL
				addOp(OP_PRINT);
			}
			| B256PRINT listitems {
				addIntOp(OP_PUSHINT, listlen); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}
			| B256PRINT '(' listitems ')' {
				addIntOp(OP_PUSHINT, listlen); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}
			| '?' args_none {
				addIntOp(OP_PUSHINT, 0); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}
			| '?' expr B256SEMICOLON {
				addIntOp(OP_PUSHINT, 1); //push number of arguments passed
				addIntOp(OP_PUSHINT, 0); // suppress NL
				addOp(OP_PRINT);
			}
			| '?' listitems {
				addIntOp(OP_PUSHINT, listlen); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}
			| '?' '(' listitems ')' {
				addIntOp(OP_PUSHINT, listlen); //push number of arguments passed
				addIntOp(OP_PUSHINT, 1); // need NL
				addOp(OP_PRINT);
			}			;

wavpausestmt:
			B256WAVPAUSE args_none {
				addOp(OP_WAVPAUSE);
			}
			;

wavplaystmt:
			B256WAVPLAY args_none {
				addStringOp(OP_PUSHSTRING, "");
				addOp(OP_WAVPLAY);
			}
			| B256WAVPLAY expr {
				addOp(OP_WAVPLAY);
			}
			;
wavseekstmt:
			B256WAVSEEK expr {
				addOp(OP_WAVSEEK);
			}
			;

wavstopstmt:
			B256WAVSTOP args_none {
				addOp(OP_WAVSTOP);
			}
			;

wavwaitstmt:
			B256WAVWAIT args_none {
				addOp(OP_WAVWAIT);
			}
			;

putslicestmt:
			B256PUTSLICE args_eee  {
				addOp(OP_PUTSLICE);
			}
			;

imgloadstmt:
			B256IMGLOAD args_eee
			{
				addIntOp(OP_PUSHINT, 1); // scale
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_eeee
			{
				addIntOp(OP_PUSHINT, 0); // rotate
				addOp(OP_STACKSWAP);
				addOp(OP_IMGLOAD);
			}
			| B256IMGLOAD args_eeeee
			{
				addOp(OP_IMGLOAD);
			}
			;

spritedimstmt:
			B256SPRITEDIM expr {
				addOp(OP_SPRITEDIM);
			}
			;

spriteloadstmt:
			B256SPRITELOAD args_ee  {
				addOp(OP_SPRITELOAD);
			}
			;

spriteslicestmt:
			B256SPRITESLICE args_eeeee {
				addOp(OP_SPRITESLICE);
			}
			;

spritepolystmt:
			B256SPRITEPOLY args_ee {
				addOp(OP_SPRITEPOLY);
			}
			;

spriteplacestmt:
			B256SPRITEPLACE args_eee
			{
				addIntOp(OP_PUSHINT,3);	// nr of arguments
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_eeee
			{
				addIntOp(OP_PUSHINT,4);	// nr of arguments
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_eeeee
			{
				addIntOp(OP_PUSHINT,5);	// nr of arguments
				addOp(OP_SPRITEPLACE);
			}
			| B256SPRITEPLACE args_eeeeee
			{
					addIntOp(OP_PUSHINT,6);	// nr of arguments
					addOp(OP_SPRITEPLACE);
			}
			;

spritemovestmt:
			B256SPRITEMOVE args_eee
			{
				addIntOp(OP_PUSHINT,3);	// nr of arguments
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_eeee  {
				addIntOp(OP_PUSHINT,4);	// nr of arguments
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_eeeee {
				addIntOp(OP_PUSHINT,5);	// nr of arguments
				addOp(OP_SPRITEMOVE);
			}
			| B256SPRITEMOVE args_eeeeee {
					addIntOp(OP_PUSHINT,6);	// nr of arguments
					addOp(OP_SPRITEMOVE);
			}
			;
			
spritehidestmt:
			B256SPRITEHIDE expr {
				addOp(OP_SPRITEHIDE);
			}
			;

spriteshowstmt:
			B256SPRITESHOW expr {
				addOp(OP_SPRITESHOW);
			}
			;

clickclearstmt:
			B256CLICKCLEAR args_none {
				addOp(OP_CLICKCLEAR);
			}
			;

changedirstmt:
			B256CHANGEDIR expr {
				addOp(OP_CHANGEDIR);
			}
			;

dbopenstmt:
			B256DBOPEN expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPEN);
			}
			| B256DBOPEN args_ee {
				addOp(OP_DBOPEN);
			}
			;

dbclosestmt:
			B256DBCLOSE args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_DBCLOSE);
			}
			| B256DBCLOSE expr {
				addOp(OP_DBCLOSE);
			}
			;

dbexecutestmt:
			B256DBEXECUTE expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addOp(OP_DBEXECUTE);
			}
			| B256DBEXECUTE args_ee {
				addOp(OP_DBEXECUTE);
			}
			;

dbopensetstmt:
			B256DBOPENSET expr {
				addIntOp(OP_PUSHINT,0);	// default db number
				addOp(OP_STACKSWAP);
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_ee {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_STACKSWAP);
				addOp(OP_DBOPENSET);
			}
			| B256DBOPENSET args_eee {
				addOp(OP_DBOPENSET);
			}
			;

dbclosesetstmt:
			B256DBCLOSESET args_none {
				addIntOp(OP_PUSHINT,0);	// default db number
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET expr {
				addIntOp(OP_PUSHINT,0);	// default dbset number
				addOp(OP_DBCLOSESET);
			}
			| B256DBCLOSESET args_ee {
				addOp(OP_DBCLOSESET);
			}
			;

netlistenstmt:
			B256NETLISTEN expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETLISTEN);
			}
			| B256NETLISTEN args_ee {
				addOp(OP_NETLISTEN);
			}
			;

netconnectstmt:
			B256NETCONNECT args_ee {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKTOPTO2);
				addOp(OP_NETCONNECT);
			}
			| B256NETCONNECT args_eee {
				addOp(OP_NETCONNECT);
			}
			;

netwritestmt:
			B256NETWRITE expr {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_STACKSWAP);
				addOp(OP_NETWRITE);
			}
			| B256NETWRITE args_ee {
				addOp(OP_NETWRITE);
			}
			;

netclosestmt:
			B256NETCLOSE args_none {
				addIntOp(OP_PUSHINT, 0);
				addOp(OP_NETCLOSE);
			}
			| B256NETCLOSE expr {
				addOp(OP_NETCLOSE);
			}
			;

killstmt: 	B256KILL expr {
				addOp(OP_KILL);
			}
			;

setclipboardimagestmt:
			B256SETCLIPBOARDIMAGE expr {
				addOp(OP_SETCLIPBOARDIMAGE);
			}
			;

setclipboardstringstmt:
			B256SETCLIPBOARDSTRING expr {
				addOp(OP_SETCLIPBOARDSTRING);
			}
			;

setsettingstmt:
			B256SETSETTING args_eee {
				addOp(OP_SETSETTING);
			}
			;

portoutstmt:
			B256PORTOUT args_ee {
				addOp(OP_PORTOUT);
			}
			;

imgsavestmt:
			B256IMGSAVE expr  {
				addStringOp(OP_PUSHSTRING, "PNG");
				addOp(OP_IMGSAVE);
			}
			| B256IMGSAVE args_ee {
				addOp(OP_IMGSAVE);
			}
			;

editvisiblestmt:
			B256EDITVISIBLE expr {
				addOp(OP_EDITVISIBLE);
			}
			;

graphvisiblestmt:
			B256GRAPHVISIBLE expr {
				addOp(OP_GRAPHVISIBLE);
			}
			;

outputvisiblestmt:
			B256OUTPUTVISIBLE expr {
				addOp(OP_OUTPUTVISIBLE);
			}
			;

maintoolbarvisiblestmt:
			B256MAINTOOLBARVISIBLE expr {
				addOp(OP_MAINTOOLBARVISIBLE);
			}
			;

graphtoolbarvisiblestmt:
			B256GRAPHTOOLBARVISIBLE expr {
				addOp(OP_GRAPHTOOLBARVISIBLE);
			}
			;

outputtoolbarvisiblestmt:
			B256OUTPUTTOOLBARVISIBLE expr {
				addOp(OP_OUTPUTTOOLBARVISIBLE);
			}
			;
			
globalstmt:
			B256GLOBAL functionvariables {
				// create ops to make all of the variables listed globals
				if (numifs>0) {
					errorcode = COMPERR_GLOBALNOTHERE;
					return -1;
				}
				int t;
				for(t=numargs-1;t>=0;t--) {
					addIntOp(OP_GLOBAL, args[t]);
				}
				numargs=0;	// clear the list for next function
			}
			;

penwidthstmt:
			B256PENWIDTH expr {
				addOp(OP_PENWIDTH);
			}
			;

alertstmt:
			B256ALERT expr {
				addOp(OP_ALERT);
			}
			;

continuedostmt:
			B256CONTINUEDO {
				// find most recent DO and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEDO) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
					errorcode = COMPERR_CONTINUEDO;
					return -1;
				}
			}
			;

continueforstmt:
			B256CONTINUEFOR {
				// find most recent FOR and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEFOR) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
					errorcode = COMPERR_CONTINUEFOR;
					return -1;
				}
			}
			;

continuewhilestmt:
			B256CONTINUEWHILE {
				// find most recent WHILE and jump to CONTINUE
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEWHILE) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLCONTINUE));
				} else {
                                        errorcode = COMPERR_CONTINUEWHILE;
					return -1;
				}
			}
			;

exitdostmt:
			B256EXITDO {
				// find most recent DO and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEDO) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITDO;
					return -1;
				}
			}
			;

exitforstmt:
			B256EXITFOR {
				// find most recent FOR and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEFOR) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_EXITFOR, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITFOR;
					return -1;
				}
			}
			;

exitwhilestmt:
			B256EXITWHILE {
				// find most recent WHILE and jump to exit
				int n=numifs-1;
				while(n>=0&&iftabletype[n]!=IFTABLETYPEWHILE) {
					n--;
				}
				if (n>=0) {
					addIntOp(OP_GOTO, getInternalSymbol(iftableid[n],INTERNALSYMBOLEXIT));
				} else {
					errorcode = COMPERR_EXITWHILE;
					return -1;
				}
			}
			;

printercancelstmt:
			B256PRINTERCANCEL args_none {
				addOp(OP_PRINTERCANCEL);
			}
			;

printeroffstmt:
			B256PRINTEROFF args_none {
				addOp(OP_PRINTEROFF);
			}
			;

printeronstmt:
			B256PRINTERON args_none {
				addOp(OP_PRINTERON);
			}
			;

printerpagestmt:
			B256PRINTERPAGE args_none {
				addOp(OP_PRINTERPAGE);
			}
			;

functionstmt:
			B256FUNCTION variable functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the function - add the start to the label table
				functionDefSymbol = varnumber[--nvarnumber];
				//
				//check if name of function is already used by a label, subroutine or another function
				if (symtableaddress[functionDefSymbol] != -1) {
						errorcode = COMPERR_LABELREDEFINED;
						return -1;
				}
				//
				// create jump around function definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this function
				symtableaddress[functionDefSymbol] = wordOffset;
				symtableaddresstype[functionDefSymbol] = ADDRESSTYPE_FUNCTION;
				newIf(linenumber, IFTABLETYPEFUNCTION, -1);
				//
				// store the number of the arguments required by FUNCTION
				// to check if number of arguments passed match definition when is called
				symtableaddressargs[functionDefSymbol] = numargs;
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						addIntOp(OP_VAR_SET, args[t]);
					}
				}
				//
				// initialize return variable
				addIntOp(OP_PUSHINT, 0);
				addIntOp(OP_VAR_SET, functionDefSymbol);
				//
				numargs=0;	// clear the list for next function
			}
			;

subroutinestmt:
			B256SUBROUTINE variable functionvariablelist {
				if (numifs>0) {
					errorcode = COMPERR_FUNCTIONNOTHERE;
					return -1;
				}
				//
				// $2 is the symbol for the subroutine - add the start to the label table
				subroutineDefSymbol = varnumber[--nvarnumber];
				//
				//check if name of subroutine is already used by a label, function or another subroutine
				if (symtableaddress[subroutineDefSymbol] != -1) {
						errorcode = COMPERR_LABELREDEFINED;
						return -1;
				}
				//
				// create jump around subroutine definition (use nextifid and 0 for jump after)
				addIntOp(OP_GOTO, getInternalSymbol(nextifid,INTERNALSYMBOLEXIT));
				//
				// create the new if frame for this subroutine
				symtableaddress[subroutineDefSymbol] = wordOffset;
				symtableaddresstype[subroutineDefSymbol] = ADDRESSTYPE_SUBROUTINE;
				newIf(linenumber, IFTABLETYPESUBROUTINE, -1);
				//
				// store the number of the arguments required by SUBROUTINE
				// to check if number of arguments passed match definition when is called
				symtableaddressargs[subroutineDefSymbol] = numargs;
				//
				// add the assigns of the function arguments
				addOp(OP_INCREASERECURSE);
				{ 	int t;
					for(t=numargs-1;t>=0;t--) {
						addIntOp(OP_VAR_SET, args[t]);
					}
				}
				numargs=0;	// clear the list for next function
			}
			;

endfunctionstmt:
			B256ENDFUNCTION {
				if (numifs>0) {
				if (iftabletype[numifs-1]==IFTABLETYPEFUNCTION) {
					//
					// add return if there is not one
					addIntOp(OP_VAR_GET, functionDefSymbol);
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
					//
					// add address for jump around function definition
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					functionDefSymbol = -1;
					//
					numifs--;
				//
				} else {
					errorcode = testIfOnTableError(numincludes);
					linenumber = testIfOnTable(numincludes);
					return -1;
				}
			} else {
				errorcode = COMPERR_ENDFUNCTION;
				return -1;
			}
		}
		;

endsubroutinestmt:
		B256ENDSUBROUTINE {
			if (numifs>0) {
					if (iftabletype[numifs-1]==IFTABLETYPESUBROUTINE) {
					addOp(OP_DECREASERECURSE);
					addOp(OP_RETURN);
					//
					// add address for jump around function definition
					symtableaddress[getInternalSymbol(iftableid[numifs-1],INTERNALSYMBOLEXIT)] = wordOffset;
					subroutineDefSymbol = -1;
					//
					numifs--;
				} else {
					errorcode = testIfOnTableError(numincludes);
					linenumber = testIfOnTable(numincludes);
					return -1;
				}
			} else {
                                errorcode = COMPERR_ENDSUBROUTINE;
				return -1;
			}
		}
		;

regexminimalstmt:
	B256REGEXMINIMAL expr {
		addOp(OP_REGEXMINIMAL);
	}
	;

unassignstmt:
	B256UNASSIGN variable_a {
		addIntOp(OP_VAR_UN, varnumber[--nvarnumber]);
	}
	| B256UNASSIGN array_element {
		addIntOp(OP_ARR_UN, varnumber[--nvarnumber]);
	}
	;


variablewatchstmt:
	B256VARIABLEWATCH variable_a {
		addIntOp(OP_VARIABLEWATCH, varnumber[--nvarnumber]);
	}
	;

imagecropstmt:
	B256IMAGECROP args_eeeee {
		addOp(OP_IMAGECROP);
	}
	;

imageautocropstmt:
	B256IMAGEAUTOCROP expr {
		addIntOp(OP_PUSHINT,1);	// nr of arguments
		addOp(OP_IMAGEAUTOCROP);
	}
	| B256IMAGEAUTOCROP args_ee {
		addIntOp(OP_PUSHINT,2);	// nr of arguments
		addOp(OP_IMAGEAUTOCROP);
	}
	;

imageresizestmt:
	B256IMAGERESIZE args_eee {
		addIntOp(OP_PUSHINT,3);	// nr of arguments
		addOp(OP_IMAGERESIZE);
	}
	| B256IMAGERESIZE args_ee {
		addIntOp(OP_PUSHINT,2);	// nr of arguments
		addOp(OP_IMAGERESIZE);
	}
	;

imagesetpixelstmt:
	B256IMAGESETPIXEL args_eeee {
		addOp(OP_IMAGESETPIXEL);
	}
	| B256IMAGESETPIXEL args_eee {
		addOp(OP_GETCOLOR);
		addOp(OP_IMAGESETPIXEL);
	}
	;

imagedrawstmt:
	B256IMAGEDRAW args_eeeeee {
		addIntOp(OP_PUSHINT,6);	// nr of arguments
		addOp(OP_IMAGEDRAW);
	}
	| B256IMAGEDRAW args_eeeee {
		addIntOp(OP_PUSHINT,5);	// nr of arguments
		addOp(OP_IMAGEDRAW);
	}
	| B256IMAGEDRAW args_eeee {
		addIntOp(OP_PUSHINT,4);	// nr of arguments
		addOp(OP_IMAGEDRAW);
	}
	| B256IMAGEDRAW args_eee {
		addIntOp(OP_PUSHINT,3);	// nr of arguments
		addOp(OP_IMAGEDRAW);
	}
	;

imagecenteredstmt:
	B256IMAGECENTERED args_eeeeee
	{
		addIntOp(OP_PUSHINT,6);	// nr of arguments
		addOp(OP_IMAGECENTERED);
	}
	| B256IMAGECENTERED args_eeeee  {
		addIntOp(OP_PUSHINT,5);	// nr of arguments
		addOp(OP_IMAGECENTERED);
	}
	| B256IMAGECENTERED args_eeee {
		addIntOp(OP_PUSHINT,4);	// nr of arguments
		addOp(OP_IMAGECENTERED);
	}
	| B256IMAGECENTERED args_eee {
		addIntOp(OP_PUSHINT,3);	// nr of arguments
		addOp(OP_IMAGECENTERED);
	}
	;

imagetransformedstmt:
	B256IMAGETRANSFORMED args_eeeeeeeeee {
		addOp(OP_IMAGETRANSFORMED);
	}
	| B256IMAGETRANSFORMED args_eeeeeeeee {
		addIntOp(OP_PUSHINT,1); // opacity
		addOp(OP_IMAGETRANSFORMED);
	}
	;

imagerotatestmt:
	B256IMAGEROTATE args_ee {
		addOp(OP_IMAGEROTATE);
	}
	;


imageflipstmt:
	B256IMAGEFLIP args_eee {
		addOp(OP_IMAGEFLIP);
	}
	| B256IMAGEFLIP args_ee {
		addIntOp(OP_PUSHINT,0);
		addOp(OP_IMAGEFLIP);
	}
	;

imagesmoothstmt:
	B256IMAGESMOOTH expr {
		addOp(OP_IMAGESMOOTH);
	}
	;

unloadstmt:
	B256UNLOAD expr {
		addOp(OP_UNLOAD);
	}
	;

setgraphstmt:
	B256SETGRAPH expr {
		addOp(OP_SETGRAPH);
	}
	| B256SETGRAPH args_none {
		addStringOp(OP_PUSHSTRING, "");
		addOp(OP_SETGRAPH);
	}
	;

arraybasestmt:
	B256ARRAYBASE expr {
		addOp(OP_ARRAYBASE);
	}
	;



/* ####################################
   ### INSERT NEW Statements BEFORE ###
   #################################### */



%%

int
yyerror(const char *msg) {
        (void) msg;
        errorcode = COMPERR_SYNTAX;
	return -1;
}
