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

#include "InterpreterPrivate.h"

Error *error;	// define the extern here

Interpreter::Interpreter(QLocale *applocale, GraphicsBuffer *appgraphics, BasicKeyboard *appbasicKeyboard)
	: media(downloader, [this](const QString &id, QByteArray *bytes) { registerSoundBytes(id, bytes); }),
	  sprites(appgraphics),
	  fileSecurity([this](const QString &what, const QString &resolved) { return askAllowFile(what, resolved); }) {
	//yydebug = 1;
	fastgraphics = false;
	frameRateSet = false;
	windowActive = false;
	winX1 = winY1 = winX2 = winY2 = 0.0;
	windowTransform.reset();
	windowInverse.reset();
	status = R_STOPPED;
	printing = false;
	sleeper = new Sleeper();
	error = new Error();
	locale = applocale;
	graphics = appgraphics;
	basicKeyboard = appbasicKeyboard;
	downloader = NULL;
	sys = NULL;
	sockets.resize(NUMSOCKETS);
	for (int i = 0; i < NUMSOCKETS; i++)
    	sockets[i] = nullptr;

	// arrays to return warnings from compiler
	listenServer = nullptr;

#ifdef WIN32PORTIO
	// initialize the inpout32 dll
	inpout32dll  = LoadLibrary(L"inpout32.dll");
	if (inpout32dll==NULL) {
		emit(outputError(tr("ERROR - Unable to find inpout32.dll - direct port I/O disabled.\n")));
	} else {
		Inp32 = (InpOut32InpType) GetProcAddress(inpout32dll, "Inp32");
		if (Inp32==NULL) {
			emit(outputError(tr("ERROR - Unable to find Inp32 in inpout32.dll - direct port I/O disabled.\n")));
		}
		Out32 = (InpOut32OutType) GetProcAddress(inpout32dll, "Out32");
		if (Inp32==NULL) {
			emit(outputError(tr("ERROR - Unable to find Out32 in inpout32.dll - direct port I/O disabled.\n")));
		}
	}
#endif

}

Interpreter::~Interpreter() {
	delete downloader;
	delete sleeper;
	delete error;
}

/*
int Interpreter::optype(int op) {
	// use constantants found in WordCodes.h
	return (OPTYPE_MASK & op) ;
}
*/

QString Interpreter::opname(int op) {
	// used to convert opcode number in debuginfo to opcode name

	switch (op) {
	case OP_ABS : return QString("OP_ABS");
	case OP_ACOS : return QString("OP_ACOS");
	case OP_ADD : return QString("OP_ADD");
	case OP_ALEN : return QString("OP_ALEN");
	case OP_ALENCOLS : return QString("OP_ALENCOLS");
	case OP_ALENROWS : return QString("OP_ALENROWS");
	case OP_ALERT : return QString("OP_ALERT");
	case OP_AND : return QString("OP_AND");
	case OP_ARC : return QString("OP_ARC");
	case OP_ARRAY2STACK : return QString("OP_ARRAY2STACK");
	case OP_ARRAYADD : return QString("OP_ARRAYADD");
	case OP_ARRAYFILL : return QString("OP_ARRAYFILL");
	case OP_ARRAYLISTASSIGN : return QString("OP_ARRAYLISTASSIGN");
	case OP_ARR_ASSIGNED : return QString("OP_ARR_ASSIGNED");
	case OP_ARR_GET : return QString("OP_ARR_GET");
	case OP_ARR_SET : return QString("OP_ARR_SET");
	case OP_ARR_UN : return QString("OP_ARR_UN");
	case OP_ASC : return QString("OP_ASC");
	case OP_ASIN : return QString("OP_ASIN");
	case OP_ATAN : return QString("OP_ATAN");
	case OP_BINARYAND : return QString("OP_BINARYAND");
	case OP_BINARYNOT : return QString("OP_BINARYNOT");
	case OP_BINARYOR : return QString("OP_BINARYOR");
	case OP_BITSHIFTL : return QString("OP_BITSHIFTL");
	case OP_BITSHIFTR : return QString("OP_BITSHIFTR");
	case OP_BRANCH : return QString("OP_BRANCH");
	case OP_CALLFUNCTION : return QString("OP_CALLFUNCTION");
	case OP_CALLSUBROUTINE : return QString("OP_CALLSUBROUTINE");
	case OP_CEIL : return QString("OP_CEIL");
	case OP_CHANGEDIR : return QString("OP_CHANGEDIR");
	case OP_CHORD : return QString("OP_CHORD");
	case OP_CHR : return QString("OP_CHR");
	case OP_CIRCLE : return QString("OP_CIRCLE");
	case OP_CLG : return QString("OP_CLG");
	case OP_CLICKB : return QString("OP_CLICKB");
	case OP_CLICKCLEAR : return QString("OP_CLICKCLEAR");
	case OP_CLICKX : return QString("OP_CLICKX");
	case OP_CLICKY : return QString("OP_CLICKY");
	case OP_CLOSE : return QString("OP_CLOSE");
	case OP_CLS : return QString("OP_CLS");
	case OP_CONCATENATE : return QString("OP_CONCATENATE");
	case OP_CONFIRM : return QString("OP_CONFIRM");
	case OP_COS : return QString("OP_COS");
	case OP_CROSS : return QString("OP_CROSS");
	case OP_COUNT : return QString("OP_COUNT");
	case OP_COUNTX : return QString("OP_COUNTX");
	case OP_CURRENTDIR : return QString("OP_CURRENTDIR");
	case OP_CURRLINE : return QString("OP_CURRLINE");
	case OP_DAY : return QString("OP_DAY");
	case OP_DBCLOSE : return QString("OP_DBCLOSE");
	case OP_DBCLOSESET : return QString("OP_DBCLOSESET");
	case OP_DBEXECUTE : return QString("OP_DBEXECUTE");
	case OP_DBFLOAT : return QString("OP_DBFLOAT");
	case OP_DBINT : return QString("OP_DBINT");
	case OP_DBNULL : return QString("OP_DBNULL");
	case OP_DBNULLS : return QString("OP_DBNULLS");
	case OP_DBOPEN : return QString("OP_DBOPEN");
	case OP_DBOPENSET : return QString("OP_DBOPENSET");
	case OP_DBROW : return QString("OP_DBROW");
	case OP_DBSTRING : return QString("OP_DBSTRING");
	case OP_DEBUGINFO : return QString("OP_DEBUGINFO");
	case OP_DECREASERECURSE : return QString("OP_DECREASERECURSE");
	case OP_DEGREES : return QString("OP_DEGREES");
	case OP_DIM : return QString("OP_DIM");
	case OP_DIR : return QString("OP_DIR");
	case OP_DIV : return QString("OP_DIV");
	case OP_DOT : return QString("OP_DOT");
	case OP_EDITVISIBLE : return QString("OP_EDITVISIBLE");
	case OP_ELLIPSE : return QString("OP_ELLIPSE");
	case OP_END : return QString("OP_END");
	case OP_EOF : return QString("OP_EOF");
	case OP_EQUAL : return QString("OP_EQUAL");
	case OP_EX : return QString("OP_EX");
	case OP_EXISTS : return QString("OP_EXISTS");
	case OP_EXITFOR : return QString("OP_EXITFOR");
	case OP_EXP : return QString("OP_EXP");
	case OP_EXPLODE : return QString("OP_EXPLODE");
	case OP_EXPLODEX : return QString("OP_EXPLODEX");
	case OP_FASTGRAPHICS : return QString("OP_FASTGRAPHICS");
	case OP_FLOAT : return QString("OP_FLOAT");
	case OP_FLOOR : return QString("OP_FLOOR");
	case OP_FONT : return QString("OP_FONT");
	case OP_FOR : return QString("OP_FOR");
	case OP_FOREACH : return QString("OP_FOREACH");
	case OP_FREEDB : return QString("OP_FREEDB");
	case OP_FREEDBSET : return QString("OP_FREEDBSET");
	case OP_FREEFILE : return QString("OP_FREEFILE");
	case OP_FREENET : return QString("OP_FREENET");
	case OP_FROMRADIX : return QString("OP_FROMRADIX");
	case OP_GETARRAYBASE : return QString("OP_GETARRAYBASE");
	case OP_GETBRUSHCOLOR : return QString("OP_GETBRUSHCOLOR");
	case OP_GETCOLOR : return QString("OP_GETCOLOR");
	case OP_GETPENWIDTH : return QString("OP_GETPENWIDTH");
	case OP_GETSETTING : return QString("OP_GETSETTING");
	case OP_GETSLICE : return QString("OP_GETSLICE");
	case OP_GLOBAL : return QString("OP_GLOBAL");
	case OP_GOSUB : return QString("OP_GOSUB");
	case OP_GOTO : return QString("OP_GOTO");
	case OP_GRAPHHEIGHT : return QString("OP_GRAPHHEIGHT");
	case OP_GRAPHSIZE : return QString("OP_GRAPHSIZE");
	case OP_GRAPHVISIBLE : return QString("OP_GRAPHVISIBLE");
	case OP_GRAPHTOOLBARVISIBLE : return QString("OP_GRAPHTOOLBARVISIBLE");
	case OP_GRAPHWIDTH : return QString("OP_GRAPHWIDTH");
	case OP_GT : return QString("OP_GT");
	case OP_GTE : return QString("OP_GTE");
	case OP_HOUR : return QString("OP_HOUR");
	case OP_IMAGEAUTOCROP : return QString("OP_IMAGEAUTOCROP");
	case OP_IMAGECENTERED : return QString("OP_IMAGECENTERED");
	case OP_IMAGECOPY : return QString("OP_IMAGECOPY");
	case OP_IMAGECROP : return QString("OP_IMAGECROP");
	case OP_IMAGEDRAW : return QString("OP_IMAGEDRAW");
	case OP_IMAGEFLIP : return QString("OP_IMAGEFLIP");
	case OP_IMAGEHEIGHT : return QString("OP_IMAGEHEIGHT");
	case OP_IMAGELOAD : return QString("OP_IMAGELOAD");
	case OP_IMAGENEW : return QString("OP_IMAGENEW");
	case OP_IMAGEPIXEL : return QString("OP_IMAGEPIXEL");
	case OP_IMAGERESIZE : return QString("OP_IMAGERESIZE");
	case OP_IMAGEROTATE : return QString("OP_IMAGEROTATE");
	case OP_IMAGESETPIXEL : return QString("OP_IMAGESETPIXEL");
	case OP_IMAGESMOOTH : return QString("OP_IMAGESMOOTH");
	case OP_IMAGETRANSFORMED : return QString("OP_IMAGETRANSFORMED");
	case OP_IMAGEWIDTH : return QString("OP_IMAGEWIDTH");
	case OP_IMGLOAD : return QString("OP_IMGLOAD");
	case OP_IMGSAVE : return QString("OP_IMGSAVE");
	case OP_IMPLODE : return QString("OP_IMPLODE");
	case OP_IN : return QString("OP_IN");
	case OP_INCREASERECURSE : return QString("OP_INCREASERECURSE");
	case OP_INPUT : return QString("OP_INPUT");
	case OP_INSTR : return QString("OP_INSTR");
	case OP_INSTRX : return QString("OP_INSTRX");
	case OP_INT : return QString("OP_INT");
	case OP_INTDIV : return QString("OP_INTDIV");
	case OP_ISNUMERIC : return QString("OP_ISNUMERIC");
	case OP_KEY : return QString("OP_KEY");
	case OP_KEYPRESSED : return QString("OP_KEYPRESSED");
	case OP_KILL : return QString("OP_KILL");
	case OP_LASTERROR : return QString("OP_LASTERROR");
	case OP_LASTERROREXTRA : return QString("OP_LASTERROREXTRA");
	case OP_LASTERRORLINE : return QString("OP_LASTERRORLINE");
	case OP_LASTERRORMESSAGE : return QString("OP_LASTERRORMESSAGE");
	case OP_LEFT : return QString("OP_LEFT");
	case OP_LENGTH : return QString("OP_LENGTH");
	case OP_LINE : return QString("OP_LINE");
	case OP_LIST2ARRAY : return QString("OP_LIST2ARRAY");
	case OP_LIST2MAP : return QString("OP_LIST2MAP");
	case OP_LOCATE : return QString("OP_LOCATE");
	case OP_TEXTSCREEN : return QString("OP_TEXTSCREEN");
	case OP_TEXTCHAR : return QString("OP_TEXTCHAR");
	case OP_HSV : return QString("OP_HSV");
	case OP_LOG : return QString("OP_LOG");
	case OP_LOGTEN : return QString("OP_LOGTEN");
	case OP_LOWER : return QString("OP_LOWER");
	case OP_LT : return QString("OP_LT");
	case OP_LTE : return QString("OP_LTE");
	case OP_LTRIM : return QString("OP_LTRIM");
	case OP_MAINTOOLBARVISIBLE : return QString("OP_MAINTOOLBARVISIBLE");
	case OP_MAP_DIM : return QString("OP_MAP_DIM");
	case OP_MAXIMIZE : return QString("OP_MAXIMIZE");
	case OP_MATADD : return QString("OP_MATADD");
	case OP_MATINV : return QString("OP_MATINV");
	case OP_MATMUL : return QString("OP_MATMUL");
	case OP_MATSUB : return QString("OP_MATSUB");
	case OP_MATTRN : return QString("OP_MATTRN");
	case OP_WINDOW : return QString("OP_WINDOW");
	case OP_MD5 : return QString("OP_MD5");
	case OP_MID : return QString("OP_MID");
	case OP_MIDX : return QString("OP_MIDX");
	case OP_MINUTE : return QString("OP_MINUTE");
	case OP_MKDIR : return QString("OP_MKDIR");
	case OP_MOD : return QString("OP_MOD");
	case OP_MONTH : return QString("OP_MONTH");
	case OP_MOUSEB : return QString("OP_MOUSEB");
	case OP_MOUSEX : return QString("OP_MOUSEX");
	case OP_MOUSEY : return QString("OP_MOUSEY");
	case OP_MSEC : return QString("OP_MSEC");
	case OP_MUL : return QString("OP_MUL");
	case OP_NEGATE : return QString("OP_NEGATE");
	case OP_NEQUAL : return QString("OP_NEQUAL");
	case OP_NETADDRESS : return QString("OP_NETADDRESS");
	case OP_NETCLOSE : return QString("OP_NETCLOSE");
	case OP_NETCONNECT : return QString("OP_NETCONNECT");
	case OP_NETDATA : return QString("OP_NETDATA");
	case OP_NETLISTEN : return QString("OP_NETLISTEN");
	case OP_NETREAD : return QString("OP_NETREAD");
	case OP_NETWRITE : return QString("OP_NETWRITE");
	case OP_NEXT : return QString("OP_NEXT");
	case OP_NOP : return QString("OP_NOP");
	case OP_NOT : return QString("OP_NOT");
	case OP_OFFERROR : return QString("OP_OFFERROR");
	case OP_OFFERRORCATCH : return QString("OP_OFFERRORCATCH");
	case OP_ONERRORCALL : return QString("OP_ONERRORCALL");
	case OP_ONERRORCATCH : return QString("OP_ONERRORCATCH");
	case OP_ONERRORGOSUB : return QString("OP_ONERRORGOSUB");
	case OP_ONSTOPCALL : return QString("OP_ONSTOPCALL");
	case OP_OPEN : return QString("OP_OPEN");
	case OP_OPENFILEDIALOG : return QString("OP_OPENFILEDIALOG");
	case OP_OPENSERIAL : return QString("OP_OPENSERIAL");
	case OP_OR : return QString("OP_OR");
	case OP_OSTYPE : return QString("OP_OSTYPE");
	case OP_OUTPUTVISIBLE : return QString("OP_OUTPUTVISIBLE");
	case OP_OUTPUTTOOLBARVISIBLE : return QString("OP_OUTPUTTOOLBARVISIBLE");
	case OP_PAUSE : return QString("OP_PAUSE");
	case OP_PENWIDTH : return QString("OP_PENWIDTH");
	case OP_PIE : return QString("OP_PIE");
	case OP_PIXEL : return QString("OP_PIXEL");
	case OP_PLOT : return QString("OP_PLOT");
	case OP_POLY : return QString("OP_POLY");
	case OP_PORTIN : return QString("OP_PORTIN");
	case OP_PORTOUT : return QString("OP_PORTOUT");
	case OP_PRINT : return QString("OP_PRINT");
	case OP_PRINTERCANCEL : return QString("OP_PRINTERCANCEL");
	case OP_PRINTEROFF : return QString("OP_PRINTEROFF");
	case OP_PRINTERON : return QString("OP_PRINTERON");
	case OP_PRINTERPAGE : return QString("OP_PRINTERPAGE");
	case OP_PROMPT : return QString("OP_PROMPT");
	case OP_PUSHFLOAT : return QString("OP_PUSHFLOAT");
	case OP_PUSHINT : return QString("OP_PUSHINT");
	case OP_PUSHLONG: return QString("OP_PUSHLONG");
	case OP_PUSHLABEL : return QString("OP_PUSHLABEL");
	case OP_PUSHSTRING : return QString("OP_PUSHSTRING");
	case OP_PUTSLICE : return QString("OP_PUTSLICE");
	case OP_RADIANS : return QString("OP_RADIANS");
	case OP_NOISE : return QString("OP_NOISE");
	case OP_NORM : return QString("OP_NORM");
	case OP_RAND : return QString("OP_RAND");
	case OP_READ : return QString("OP_READ");
	case OP_READBYTE : return QString("OP_READBYTE");
	case OP_READLINE : return QString("OP_READLINE");
	case OP_RECT : return QString("OP_RECT");
	case OP_REDIM : return QString("OP_REDIM");
	case OP_REFRESH : return QString("OP_REFRESH");
	case OP_REGEXMINIMAL : return QString("OP_REGEXMINIMAL");
	case OP_REPLACE : return QString("OP_REPLACE");
	case OP_REPLACEX : return QString("OP_REPLACEX");
	case OP_RESET : return QString("OP_RESET");
	case OP_RETURN : return QString("OP_RETURN");
	case OP_RGB : return QString("OP_RGB");
	case OP_RIGHT : return QString("OP_RIGHT");
	case OP_ROUNDEDRECT : return QString("OP_ROUNDEDRECT");
	case OP_RTRIM : return QString("OP_RTRIM");
	case OP_SAVEFILEDIALOG : return QString("OP_SAVEFILEDIALOG");
	case OP_SAY : return QString("OP_SAY");
	case OP_SECOND : return QString("OP_SECOND");
	case OP_SEED : return QString("OP_SEED");
	case OP_SEEK : return QString("OP_SEEK");
	case OP_SERIALIZE : return QString("OP_SERIALIZE");
	case OP_SETCOLOR : return QString("OP_SETCOLOR");
	case OP_SETGRAPH : return QString("OP_SETGRAPH");
	case OP_SETSETTING : return QString("OP_SETSETTING");
	case OP_SIN : return QString("OP_SIN");
	case OP_SIZE : return QString("OP_SIZE");
	case OP_SORT : return QString("OP_SORT");
	case OP_SOUND : return QString("OP_SOUND");
	case OP_SOUNDENVELOPE : return QString("OP_SOUNDENVELOPE");
	case OP_SOUNDFADE : return QString("OP_SOUNDFADE");
	case OP_SOUNDHARMONICS : return QString("OP_SOUNDHARMONICS");
	case OP_SOUNDHARMONICS_A : return QString("OP_SOUNDHARMONICS_A");
	case OP_SOUNDID : return QString("OP_SOUNDID");
	case OP_SOUNDLENGTH : return QString("OP_SOUNDLENGTH");
	case OP_SOUNDLOAD : return QString("OP_SOUNDLOAD");
	case OP_SOUNDLOADRAW : return QString("OP_SOUNDLOADRAW");
	case OP_SOUNDLOOP : return QString("OP_SOUNDLOOP");
	case OP_SOUNDNOHARMONICS : return QString("OP_SOUNDNOHARMONICS");
	case OP_SOUNDPAUSE : return QString("OP_SOUNDPAUSE");
	case OP_SOUNDPLAY : return QString("OP_SOUNDPLAY");
	case OP_SOUNDPLAYER : return QString("OP_SOUNDPLAYER");
	case OP_SOUNDPLAYEROFF : return QString("OP_SOUNDPLAYEROFF");
	case OP_SOUNDPOSITION : return QString("OP_SOUNDPOSITION");
	case OP_SOUNDSAMPLERATE : return QString("OP_SOUNDSAMPLERATE");
	case OP_SOUNDSEEK : return QString("OP_SOUNDSEEK");
	case OP_SOUNDSTATE : return QString("OP_SOUNDSTATE");
	case OP_SOUNDSTOP : return QString("OP_SOUNDSTOP");
	case OP_SOUNDSYSTEM : return QString("OP_SOUNDSYSTEM");
	case OP_SOUNDVOLUME : return QString("OP_SOUNDVOLUME");
	case OP_SOUNDWAIT : return QString("OP_SOUNDWAIT");
	case OP_SOUNDWAVEFORM : return QString("OP_SOUNDWAVEFORM");
	case OP_SPRITECOLLIDE : return QString("OP_SPRITECOLLIDE");
	case OP_SPRITEDIM : return QString("OP_SPRITEDIM");
	case OP_SPRITEH : return QString("OP_SPRITEH");
	case OP_SPRITEHIDE : return QString("OP_SPRITEHIDE");
	case OP_SPRITELOAD : return QString("OP_SPRITELOAD");
	case OP_SPRITEMOVE : return QString("OP_SPRITEMOVE");
	case OP_SPRITEO : return QString("OP_SPRITEO");
	case OP_SPRITEPLACE : return QString("OP_SPRITEPLACE");
	case OP_SPRITEPOLY : return QString("OP_SPRITEPOLY");
	case OP_SPRITER : return QString("OP_SPRITER");
	case OP_SPRITES : return QString("OP_SPRITES");
	case OP_SPRITESHOW : return QString("OP_SPRITESHOW");
	case OP_SPRITESLICE : return QString("OP_SPRITESLICE");
	case OP_SPRITETEXT : return QString("OP_SPRITETEXT");
	case OP_SPRITEV : return QString("OP_SPRITEV");
	case OP_SPRITEW : return QString("OP_SPRITEW");
	case OP_SPRITEX : return QString("OP_SPRITEX");
	case OP_SPRITEY : return QString("OP_SPRITEY");
	case OP_SQR : return QString("OP_SQR");
	case OP_STACKDUP : return QString("OP_STACKDUP");
	case OP_STACKDUP2 : return QString("OP_STACKDUP2");
	case OP_STACKSAVE : return QString("OP_STACKSAVE");
	case OP_STACKSWAP : return QString("OP_STACKSWAP");
	case OP_STACKSWAP2 : return QString("OP_STACKSWAP2");
	case OP_STACKTOPTO2 : return QString("OP_STACKTOPTO2");
	case OP_STACKUNSAVE : return QString("OP_STACKUNSAVE");
	case OP_STAMP : return QString("OP_STAMP");
	case OP_STRING : return QString("OP_STRING");
	case OP_SUB : return QString("OP_SUB");
	case OP_SYSTEM : return QString("OP_SYSTEM");
	case OP_TAN : return QString("OP_TAN");
	case OP_TEXT : return QString("OP_TEXT");
	case OP_TEXTBOXHEIGHT : return QString("OP_TEXTBOXHEIGHT");
	case OP_TEXTBOXWIDTH : return QString("OP_TEXTBOXWIDTH");
	case OP_TEXTCOLOR : return QString("OP_TEXTCOLOR");
	case OP_TEXTCOL : return QString("OP_TEXTCOL");
	case OP_TEXTBACKGROUND : return QString("OP_TEXTBACKGROUND");
	case OP_TEXTFONT : return QString("OP_TEXTFONT");
	case OP_TEXTROW : return QString("OP_TEXTROW");
	case OP_TEXTHEIGHT : return QString("OP_TEXTHEIGHT");
	case OP_TEXTWIDTH : return QString("OP_TEXTWIDTH");
	case OP_THROWERROR : return QString("OP_THROWERROR");
	case OP_TORADIX : return QString("OP_TORADIX");
	case OP_TRIM : return QString("OP_TRIM");
	case OP_TYPEOF : return QString("OP_TYPEOF");
	case OP_UNLOAD : return QString("OP_UNLOAD");
	case OP_UNIT : return QString("OP_UNIT");
	case OP_FRAMERATE : return QString("OP_FRAMERATE");
	case OP_UNSERIALIZE : return QString("OP_UNSERIALIZE");
	case OP_UPPER : return QString("OP_UPPER");
	case OP_VARIABLECOPY : return QString("OP_VARIABLECOPY");
	case OP_VARIABLEWATCH : return QString("OP_VARIABLEWATCH");
	case OP_VAR_ASSIGNED : return QString("OP_VAR_ASSIGNED");
	case OP_VAR_GET : return QString("OP_VAR_GET");
	case OP_VAR_REF : return QString("OP_VAR_REF");
	case OP_VAR_SET : return QString("OP_VAR_SET");
	case OP_VAR_UN : return QString("OP_VAR_UN");
	case OP_VOLUME : return QString("OP_VOLUME");
	case OP_WAVLENGTH : return QString("OP_WAVLENGTH");
	case OP_WAVPAUSE : return QString("OP_WAVPAUSE");
	case OP_WAVPLAY : return QString("OP_WAVPLAY");
	case OP_WAVPOS : return QString("OP_WAVPOS");
	case OP_WAVSEEK : return QString("OP_WAVSEEK");
	case OP_WAVSTATE : return QString("OP_WAVSTATE");
	case OP_WAVSTOP : return QString("OP_WAVSTOP");
	case OP_WAVWAIT : return QString("OP_WAVWAIT");
	case OP_WRITE : return QString("OP_WRITE");
	case OP_WRITEBYTE : return QString("OP_WRITEBYTE");
	case OP_WRITELINE : return QString("OP_WRITELINE");
	case OP_XOR : return QString("OP_XOR");
	case OP_YEAR : return QString("OP_YEAR");
	case OP_SETCLIPBOARDIMAGE : return QString("OP_SETCLIPBOARDIMAGE");
	case OP_SETCLIPBOARDSTRING : return QString("OP_SETCLIPBOARDSTRING");
	case OP_GETCLIPBOARDIMAGE : return QString("OP_GETCLIPBOARDIMAGE");
	case OP_GETCLIPBOARDSTRING : return QString("OP_GETCLIPBOARDSTRING");

	default: return QString("OP_UNKNOWN");
	}
}

void Interpreter::printError() {
	QString msg;
	if (error->isFatal()) {
		msg = tr("ERROR");
	} else {
		msg = tr("WARNING");
	}
	if (includeFileNumber!=0) {
		msg += tr(" in included file '") + include_filenames[includeFileNumber] + QStringLiteral("'");
	}
	msg += tr(" on line ") + QString::number(error->line) + QStringLiteral(": ") + error->getErrorMessage(symtable);
	msg += QStringLiteral(".\n");
	emit(outputError(msg));
}


void Interpreter::netSockClose(int fn)
{
#ifdef BASIC256_ENABLE_TCP
    // fn is the BASIC256 socket slot index (0 to NUMSOCKETS-1)
    if (fn >= 0 && fn < sockets.size() && sockets[fn]) {
        sockets[fn]->disconnectFromHost();
        sockets[fn]->close();
        delete sockets[fn];
        sockets[fn] = nullptr;
    }
#else
    (void)fn;
#endif
}

void Interpreter::netSockCloseAll()
{
#ifdef BASIC256_ENABLE_TCP
    if (listenServer) {
        listenServer->close();
        delete listenServer;
        listenServer = nullptr;
    }

    for (int t = 0; t < sockets.size(); t++) {
        netSockClose(t);   // reuse the single-close logic, don't duplicate it
    }
#endif
}

void Interpreter::setInputString(QString s) {
	inputString = s;
}

bool Interpreter::isRunning() {
	return (status != R_STOPPED);
}

bool Interpreter::isStopped() {
	return (status == R_STOPPED);
}

bool Interpreter::isStopping() {
	// interpreter is stopped or is about to stop
	// to avoid RunController::stopRun() to be triggered while status == R_STOPPING too
	return (status == R_STOPPED || status == R_STOPPING);
}

void Interpreter::wakeSleeper() {
	// Called from the GUI thread when the user presses Stop. The interpreter
	// thread may be parked in a PAUSE or a FRAMERATE wait, neither of which
	// the status flag alone can reach -- the run loop only looks at the status
	// between opcodes, so without this a PAUSE 60 would ignore Stop for a
	// minute.
	sleeper->wake();
}

void Interpreter::setStatus(run_status s) {
	status = s;
}

void Interpreter::watchvariable(bool doit, int i) {
	// send an event to the variable watch window to display a variable/array content
	if (doit) {
		int level = variables->getrecurse();
		int varnum = i;
		Variable* v = variables->getAt(i, level);
		while (DataElement::getType(v->data) == T_REF) {
			emit(varWinAssign(&variables, varnum, level));
			varnum = v->data->intval;
			level = v->data->level;
			v = variables->getAt(i, level);
		}
		emit(varWinAssign(&variables, varnum, level));
	}
}
void Interpreter::watchvariable(bool doit, int i, int x, int y) {
	// send an event to the variable watch window to display aan array element's value
	if (doit) {
		int level = variables->getrecurse();
		int varnum = i;
		Variable* v = variables->getAt(i, level);
		while (DataElement::getType(v->data) == T_REF) {
			emit(varWinAssign(&variables, varnum, level));
			varnum = v->data->intval;
			level = v->data->level;
			v = variables->getAt(i, level);
		}
		emit(varWinAssign(&variables, varnum, level, x ,y));
	}
}
void Interpreter::watchvariable(bool doit, int i, QString k) {
	// send an event to the variable watch window to display a map element's value
	if (doit) {
		int level = variables->getrecurse();
		int varnum = i;
		Variable* v = variables->getAt(i, level);
		while (DataElement::getType(v->data) == T_REF) {
			emit(varWinAssign(&variables, varnum, level));
			varnum = v->data->intval;
			level = v->data->level;
			v = variables->getAt(i, level);
		}
		emit(varWinAssign(&variables, varnum, level, k));
	}
}

void Interpreter::watchdecurse(bool doit) {
	// send an event to the variable watch window to remove a function's variables
	if (doit) {
		emit(varWinDropLevel(variables->getrecurse()));
	}
}

// ---------------------------------------------------------------------------
// MAT - matrix arithmetic over BASIC-256 arrays
//
// An array is read as rows by columns, the way DIM writes it: DIM a(200,2) is
// 200 rows of 2 columns, arrayRows() gives the rows and arrayCols() the
// columns, and element (r,c) sits at arr->data[r*ydim+c].  Every MAT statement
// uses that one reading, so a matrix of particle positions is 200 rows of an x
// and a y.
//
// Element arithmetic follows the rules an ordinary "a + b" follows: two whole
// numbers give a whole number unless the answer leaves the 32-bit range
// BASIC-256 promotes at, and anything else is worked out in floating point.
// Integer matrices therefore give exactly the answers a textbook does.
//
// The loops below run over the array's own storage, so nothing goes back
// through the interpreter for an element, and an operation that can be done
// element by element writes its answer straight into the DataElements the
// destination already holds.  MAT ADD Position = Position + Velocity over 200
// particles therefore allocates nothing at all, and is still correct when the
// destination is one of the operands.  Only the operations whose answer
// depends on more than one element of a source - MUL between two matrices,
// TRN onto itself, and INV - build a working copy first.
// ---------------------------------------------------------------------------

namespace {

	// one number taken out of a matrix element, kept whole while it can be
	struct MatNum {
		bool isint;
		qint64 i;
		double d;
		double f() const {
			return isint ? (double) i : d;
		}
	};

	inline MatNum matInt(qint64 v) {
		MatNum n;
		n.isint = true;
		n.i = v;
		n.d = 0.0;
		return n;
	}

	inline MatNum matFloat(double v) {
		MatNum n;
		n.isint = false;
		n.i = 0;
		n.d = v;
		return n;
	}

	// the three below mirror OP_ADD, OP_SUB and OP_MUL exactly, including where
	// each of them gives up on whole numbers and promotes to floating point
	inline MatNum matNumAdd(const MatNum &a, const MatNum &b) {
		if (a.isint && b.isint) {
			qint64 v = a.i + b.i;
			if (v>=INT_MIN && v<=INT_MAX) return matInt(v);
		}
		return matFloat(a.f() + b.f());
	}

	inline MatNum matNumSub(const MatNum &a, const MatNum &b) {
		if (a.isint && b.isint) {
			qint64 v = a.i - b.i;
			if (v>=INT_MIN && v<=INT_MAX) return matInt(v);
		}
		return matFloat(a.f() - b.f());
	}

	inline MatNum matNumMul(const MatNum &a, const MatNum &b) {
		if (a.isint && b.isint) {
			if (a.i==0 || b.i==0) return matInt(0);
			if (llabs(a.i) <= INT64_MAX / llabs(b.i)) {
				qint64 v = a.i * b.i;
				if (v>=INT_MIN && v<=INT_MAX) return matInt(v);
			}
		}
		return matFloat(a.f() * b.f());
	}

	// write one element, reusing the DataElement that is already there
	inline void matPut(DataElement *e, const MatNum &v) {
		e->clear();
		if (v.isint) {
			e->type = T_INT;
			e->intval = v.i;
		} else {
			e->type = T_FLOAT;
			e->floatval = v.d;
		}
	}

	MatNum matGet(DataElement *m, int k, int varnum, int arraybase, Convert *convert) {
		// read element k of a matrix storage vector.  An element that has never
		// been given a value is an error naming the variable and the element,
		// just as reading it with an index would be; a string element converts
		// the way it would anywhere else, warning and all.
		DataElement *e = &m->arr->data[k];
		if (e && e->type==T_INT) return matInt(e->intval);
		if (e && e->type==T_FLOAT) return matFloat(e->floatval);
		if (!e || e->type==T_UNASSIGNED) {
			error->q(ERROR_VARNOTASSIGNED, varnum, k / m->arr->ydim + arraybase, k % m->arr->ydim + arraybase);
			return matInt(0);
		}
		if (e->type==T_ARRAY || e->type==T_MAP) {
			error->q(ERROR_NUMBEREXPR, varnum, k / m->arr->ydim + arraybase, k % m->arr->ydim + arraybase);
			return matInt(0);
		}
		return matFloat(convert->getFloat(e));
	}

	bool matSource(DataElement *e, int varnum) {
		// a MAT source has to be an array - say which variable is not one
		if (DataElement::getType(e)==T_ARRAY) return true;
		error->q(DataElement::getType(e)==T_UNASSIGNED ? ERROR_VARNOTASSIGNED : ERROR_MATNOTMATRIX, varnum);
		return false;
	}

	bool matDestination(DataElement *dest, int rows, int cols) {
		// give the destination the shape of the answer.  When it already has
		// that shape - which is what a simulation loop finds every frame after
		// the first - nothing is allocated and nothing is thrown away.
		if (DataElement::getType(dest)!=T_ARRAY || dest->arr->xdim!=rows || dest->arr->ydim!=cols) {
			dest->arrayDim(rows, cols, false);
			if (DataElement::getError()) {
				error->q(DataElement::getError(true));
				return false;
			}
		}
		// the elements exist as soon as the array does - nothing to allocate
		return true;
	}

	// DOT, CROSS, NORM and UNIT read a vector the way MAT reads a matrix -
	// through MatNum, so whole numbers stay whole - but their operands are
	// values on the stack rather than named variables, so an element that
	// will not do has to name itself by position instead of by variable name.
	MatNum vecGet(DataElement *v, int k, Convert *convert) {
		DataElement *e = &v->arr->data[k];
		if (e->type==T_INT) return matInt(e->intval);
		if (e->type==T_FLOAT) return matFloat(e->floatval);
		if (e->type==T_UNASSIGNED) {
			error->q(ERROR_VECELEMENT, QString("element %1").arg(k));
			return matInt(0);
		}
		if (e->type==T_ARRAY || e->type==T_MAP) {
			error->q(ERROR_NUMBEREXPR, QString("element %1").arg(k));
			return matInt(0);
		}
		return matFloat(convert->getFloat(e));
	}

	// a vector operand is any array - a row, a column, or for that matter a
	// whole matrix, which is read as its elements in the order they are
	// stored.  What matters to DOT and CROSS is how many elements there are,
	// not what shape they are held in, so MAT TRN output works as it stands.
	bool vecOperand(DataElement *e) {
		if (DataElement::getType(e)==T_ARRAY) return true;
		// an unassigned variable has already reported itself from OP_VAR_GET,
		// and Error keeps the first error of an operation, so saying this as
		// well costs nothing and covers the case where that one is a warning
		error->q(ERROR_VECNOTVECTOR);
		return false;
	}

	inline int vecSize(DataElement *e) {
		return e->arr->xdim * e->arr->ydim;
	}

	// read a whole vector out into MatNums.  Both operands are always read
	// before anything is pushed, because a push may reuse the very stack slot
	// an operand is still being read from.
	bool vecRead(DataElement *v, std::vector<MatNum> &out, Convert *convert) {
		const int n = vecSize(v);
		out.resize(n);
		for (int k=0; k<n; k++) out[k] = vecGet(v, k, convert);
		return !error->pending();
	}
}

void Interpreter::matStatement(int opcode, int destvar, DataElement *left, int leftvar, DataElement *right, int rightvar) {
	// left is the source matrix and right the second operand - another matrix,
	// a single number, or NULL for the one operand statements.  All three of
	// the variables involved may be the same variable.

	if (!matSource(left, leftvar)) return;
	const int arows = left->arr->xdim;
	const int acols = left->arr->ydim;
	const int asize = arows * acols;
	DataElement *dest = variables->getData(destvar);			// DONT RELEASE

	switch (opcode) {

		case OP_MATTRN: {
			// element (c,r) of the answer is element (r,c) of the source, so
			// unlike the element by element statements this can not be written
			// over its own source unless a copy is taken first
			if (dest==left) {
				std::vector<MatNum> t(asize);
				for (int k=0; k<asize; k++) t[k] = matGet(left, k, leftvar, arraybase, convert);
				if (error->pending()) return;
				if (!matDestination(dest, acols, arows)) return;
				for (int r=0; r<arows; r++) {
					for (int c=0; c<acols; c++) {
						matPut(&dest->arr->data[c*arows+r], t[r*acols+c]);
					}
				}
			} else {
				if (!matDestination(dest, acols, arows)) return;
				for (int r=0; r<arows; r++) {
					for (int c=0; c<acols; c++) {
						matPut(&dest->arr->data[c*arows+r], matGet(left, r*acols+c, leftvar, arraybase, convert));
					}
				}
			}
		}
		break;

		case OP_MATINV: {
			// Gauss-Jordan elimination with partial pivoting on [ a | I ].
			// Worked out in floating point throughout - an inverse is not a
			// whole number matrix except by accident.
			if (arows!=acols) {
				error->q(ERROR_MATNOTSQUARE, leftvar);
				return;
			}
			const int n = arows;
			const int w = 2 * n;
			std::vector<double> m((size_t)n * w, 0.0);
			double biggest = 0.0;
			for (int r=0; r<n; r++) {
				for (int c=0; c<n; c++) {
					const double v = matGet(left, r*n+c, leftvar, arraybase, convert).f();
					m[(size_t)r*w+c] = v;
					if (fabs(v) > biggest) biggest = fabs(v);
				}
				m[(size_t)r*w+n+r] = 1.0;
			}
			if (error->pending()) return;
			// how small a pivot has to be before the matrix counts as singular,
			// measured against the size of the numbers in the matrix itself so
			// that a matrix of large values is not condemned for a pivot that
			// is small only next to them
			const double tol = (biggest>0.0 ? biggest : 1.0) * n * std::numeric_limits<double>::epsilon();
			for (int col=0; col<n; col++) {
				int piv = col;
				for (int r=col+1; r<n; r++) {
					if (fabs(m[(size_t)r*w+col]) > fabs(m[(size_t)piv*w+col])) piv = r;
				}
				if (fabs(m[(size_t)piv*w+col]) <= tol) {
					error->q(ERROR_MATSINGULAR, leftvar);
					return;
				}
				if (piv!=col) {
					for (int c=col; c<w; c++) std::swap(m[(size_t)piv*w+c], m[(size_t)col*w+c]);
				}
				const double p = m[(size_t)col*w+col];
				for (int c=col; c<w; c++) m[(size_t)col*w+c] /= p;
				for (int r=0; r<n; r++) {
					if (r==col) continue;
					const double f = m[(size_t)r*w+col];
					if (f==0.0) continue;
					for (int c=col; c<w; c++) m[(size_t)r*w+c] -= f * m[(size_t)col*w+c];
				}
			}
			if (!matDestination(dest, n, n)) return;
			for (int r=0; r<n; r++) {
				for (int c=0; c<n; c++) {
					matPut(&dest->arr->data[r*n+c], matFloat(m[(size_t)r*w+n+c]));
				}
			}
		}
		break;

		default: {
			// ADD, SUB and MUL.  A second matrix and a single number are two
			// different operations for MUL and the same shape of loop for the
			// other two, so the operand is sorted out first.
			if (DataElement::getType(right)!=T_ARRAY) {
				// a single number - read out before the destination is touched,
				// because the destination may be the very variable holding it
				if (DataElement::getType(right)==T_UNASSIGNED) {
					error->q(ERROR_VARNOTASSIGNED, rightvar);
					return;
				}
				if (DataElement::getType(right)==T_MAP) {
					error->q(ERROR_NUMBEREXPR, rightvar);
					return;
				}
				const MatNum s = (right->type==T_INT) ? matInt(right->intval) :
								 (right->type==T_FLOAT) ? matFloat(right->floatval) :
								 matFloat(convert->getFloat(right));
				if (!matDestination(dest, arows, acols)) return;
				for (int k=0; k<asize; k++) {
					const MatNum a = matGet(left, k, leftvar, arraybase, convert);
					MatNum v = (opcode==OP_MATADD) ? matNumAdd(a, s) :
							   (opcode==OP_MATSUB) ? matNumSub(a, s) : matNumMul(a, s);
					if (!v.isint && std::isinf(v.d)) {
						error->q(ERROR_INFINITY);
						v = matFloat(0.0);
					}
					matPut(&dest->arr->data[k], v);
				}
				return;
			}

			const int brows = right->arr->xdim;
			const int bcols = right->arr->ydim;

			if (opcode==OP_MATMUL) {
				// the textbook product: the answer has a row for every row of
				// the first matrix and a column for every column of the second,
				// which only works out when the first has as many columns as
				// the second has rows
				if (acols!=brows) {
					error->q(ERROR_MATMULDIM, leftvar);
					return;
				}
				// every element of the answer draws on a whole row and a whole
				// column, so both sources are copied out before the destination
				// - which may be either of them - is written
				const int bsize = brows * bcols;
				const int csize = arows * bcols;
				std::vector<MatNum> a(asize), b(bsize);
				for (int k=0; k<asize; k++) a[k] = matGet(left, k, leftvar, arraybase, convert);
				for (int k=0; k<bsize; k++) b[k] = matGet(right, k, rightvar, arraybase, convert);
				if (error->pending()) return;
				std::vector<MatNum> c(csize, matInt(0));
				// a row of the answer at a time, walking a row of each source
				// forwards - the order the storage is already in
				for (int r=0; r<arows; r++) {
					const size_t crow = (size_t)r * bcols;
					for (int k=0; k<acols; k++) {
						const MatNum &aik = a[(size_t)r*acols+k];
						if (aik.isint && aik.i==0) continue;
						const size_t brow = (size_t)k * bcols;
						for (int col=0; col<bcols; col++) {
							c[crow+col] = matNumAdd(c[crow+col], matNumMul(aik, b[brow+col]));
						}
					}
				}
				if (!matDestination(dest, arows, bcols)) return;
				bool infinite = false;
				for (int k=0; k<csize; k++) {
					if (!c[k].isint && std::isinf(c[k].d)) {
						infinite = true;
						c[k] = matFloat(0.0);
					}
					matPut(&dest->arr->data[k], c[k]);
				}
				if (infinite) error->q(ERROR_INFINITY);
				return;
			}

			// ADD and SUB element by element - element k of the answer needs
			// only element k of each source, so this is safe to write straight
			// into a destination that is one of them
			if (arows!=brows || acols!=bcols) {
				error->q(ERROR_MATDIM, leftvar);
				return;
			}
			if (!matDestination(dest, arows, acols)) return;
			for (int k=0; k<asize; k++) {
				const MatNum a = matGet(left, k, leftvar, arraybase, convert);
				const MatNum b = matGet(right, k, rightvar, arraybase, convert);
				MatNum v = (opcode==OP_MATADD) ? matNumAdd(a, b) : matNumSub(a, b);
				if (!v.isint && std::isinf(v.d)) {
					error->q(ERROR_INFINITY);
					v = matFloat(0.0);
				}
				matPut(&dest->arr->data[k], v);
			}
		}
		break;
	}
}

void Interpreter::decreaserecurse() {
	//clear current forstack
	while (forstack) {
		forframe *temp = forstack;
		forstack = temp->next;
		delete temp;
	}

	watchdecurse(debugMode);
	variables->decreaserecurse();

	//pop forstack from forstacklevel
	int recurse = variables->getrecurse();
	forstack = forstacklevel[recurse];

	//delete try/catch traps from non-existent recurse level
	while(trycatchstack && trycatchstack->recurseLevel > recurse){
		trycatchframe *temp_trycatchstack = trycatchstack;
		trycatchstack=trycatchstack->next;
		delete temp_trycatchstack;
	}
}


int Interpreter::compileProgram(char *code) {
	if (initializeBasicParse() != 0) {
		emit(outputError(tr("COMPILE ERROR") + QStringLiteral(": ") + tr("Out of memory") + QStringLiteral(".\n")));
		return -1;
	}
	// include_exec_path = QCoreApplication::applicationDirPath().toUtf8().data();
	static QByteArray execPathUtf8 = QCoreApplication::applicationDirPath().toUtf8();
	include_exec_path = execPathUtf8.data();
		int result = basicParse(code);
	//
	// display warnings from compile and free the lexing file name string
		bool gotowarning = (debugMode==0);
		// go to line only for first warning and only if program is not running in debugMode
		// because in debugMode it already call goToLine(1) at start
		for(int i=0; i<numparsewarnings; i++) {
		QString msg = tr("COMPILE WARNING");
		if (parsewarningtablelexingfilenumber[i]!=0) {
			msg += tr(" in included file '") + QString(include_filenames[parsewarningtablelexingfilenumber[i]]) + QStringLiteral("'");
		} else if(gotowarning){
			emit(goToLine(parsewarningtablelinenumber[i]));
			gotowarning = false;
		}
		msg += tr(" on line ") + QString::number(parsewarningtablelinenumber[i]) + QStringLiteral(": ");
		switch(parsewarningtable[i]) {
			case COMPWARNING_MAXIMUMWARNINGS:
				msg += tr("The maximum number of compiler warnings have been displayed");
				break;
			case COMPWARNING_DEPRECATED_FORM:
				msg += tr("Statement format has been deprecated. It is recommended that you reauthor");
				break;
			case COMPWARNING_DEPRECATED_REF:
				msg += tr("You should use REF() when passing arguments, not in the SUBROUTINE/FUNCTION definition");
				break;

			default:
				msg += tr("Unknown compiler warning #") + QString::number(parsewarningtable[i]);
		}
		msg += QStringLiteral(".\n");
		emit(outputError(msg));
		//
	}
	//
	// now display fatal error if there is one
	bool gotoerror = true; // go to line only for first error in this file
	if (result != COMPERR_NONE)	{
		QString msg = tr("COMPILE ERROR");
		if (strlen(lexingfilename)!=0) {
			msg += tr(" in included file '") + QString(lexingfilename) + QStringLiteral("'");
		} else if(gotoerror){
			emit(goToLine(linenumber));
			gotoerror = false;
		}
		msg += tr(" on line ") + QString::number(linenumber) + QStringLiteral(": ");
		switch(result) {
			case COMPERR_FUNCTIONGOTO:
				msg += tr("You may not define a label or use a GOTO or GOSUB statement in a FUNCTION/SUBROUTINE declaration");
				break;
			case COMPERR_GLOBALNOTHERE:
				msg += tr("You may not define GLOBAL variable(s) inside an IF, loop, TRY, CATCH, or FUNCTION/SUBROUTINE");
				break;
			case COMPERR_FUNCTIONNOTHERE:
				msg += tr("You may not define a FUNCTION/SUBROUTINE inside an IF, loop, TRY, CATCH, or other FUNCTION/SUBROUTINE");
				break;
			case COMPERR_ENDFUNCTION:
				msg += tr("END FUNCTION without matching FUNCTION");
				break;
			case COMPERR_ENDSUBROUTINE:
				msg += tr("END SUBROUTINE without matching SUBROUTINE");
				break;
			case COMPERR_FUNCTIONNOEND:
				msg += tr("FUNCTION without matching END FUNCTION statement");
				break;
			case COMPERR_SUBROUTINENOEND:
				msg += tr("SUBROUTINE without matching END SUBROUTINE statement");
				break;
			case COMPERR_FORNOEND:
				msg += tr("FOR without matching NEXT statement");
				break;
			case COMPERR_WHILENOEND:
				msg += tr("WHILE without matching END WHILE statement");
				break;
			case COMPERR_DONOEND:
				msg += tr("DO without matching UNTIL statement");
				break;
			case COMPERR_ELSENOEND:
				msg += tr("ELSE without matching END IF or END CASE statement");
				break;
			case COMPERR_IFNOEND:
				msg += tr("IF without matching END IF or ELSE statement");
				break;
			case COMPERR_UNTIL:
				msg += tr("UNTIL without matching DO");
				break;
			case COMPERR_ENDWHILE:
				msg += tr("END WHILE without matching WHILE");
				break;
			case COMPERR_ELSE:
				msg += tr("ELSE without matching IF");
				break;
			case COMPERR_ENDIF:
				msg += tr("END IF without matching IF");
				break;
			case COMPERR_NEXT:
				msg += tr("NEXT without matching FOR");
				break;
			case COMPERR_RETURNVALUE:
				msg += tr("RETURN with a value is only valid inside a FUNCTION");
				break;
			case COMPERR_CONTINUEDO:
				msg += tr("CONTINUE DO without matching DO");
				break;
			case COMPERR_CONTINUEFOR:
				msg += tr("CONTINUE DO without matching DO");
				break;
			case COMPERR_CONTINUEWHILE:
				msg += tr("CONTINUE WHILE without matching WHILE");
				break;
			case COMPERR_EXITDO:
				msg += tr("EXIT DO without matching DO");
				break;
			case COMPERR_EXITFOR:
				msg += tr("EXIT FOR without matching FOR");
				break;
			case COMPERR_EXITWHILE:
				msg += tr("EXIT WHILE without matching WHILE");
				break;
			case COMPERR_INCLUDEFILE:
				msg += tr("Unable to open INCLUDE file");
				break;
			case COMPERR_INCLUDEDEPTH:
				msg += tr("Maximum depth of INCLUDE files");
				break;
			case COMPERR_TRYNOEND:
				msg += tr("TRY without matching CATCH statement");
				break;
			case COMPERR_CATCH:
				msg += tr("CATCH without matching TRY statement");
				break;
			case COMPERR_CATCHNOEND:
				msg += tr("CATCH without matching ENDTRY statement");
				break;
			case COMPERR_ENDTRY:
				msg += tr("ENDTRY without matching CATCH statement");
				break;
			case COMPERR_ENDBEGINCASE:
				msg += tr("CASE without matching BEGIN CASE statement");
				break;
			case COMPERR_ENDENDCASEBEGIN:
				msg += tr("END CASE without matching BEGIN CASE statement");
				break;
			case COMPERR_ENDENDCASE:
				msg += tr("END CASE without matching CASE statement");
				break;
			case COMPERR_BEGINCASENOEND:
				msg += tr("BEGIN CASE without matching END CASE statement");
				break;
			case COMPERR_CASENOEND:
				msg += tr("CASE without next CASE or matching END CASE statement");
				break;
			case COMPERR_LABELREDEFINED:
				msg += tr("Labels, functions and subroutines must have a unique name");
				break;
			case COMPERR_NEXTWRONGFOR:
				msg += tr("Variable in NEXT does not match FOR");
				break;
			case COMPERR_INCLUDEMAX:
				msg += tr("Maximum number of INCLUDE files");
				break;
			case COMPERR_INCLUDENOTALONE:
				msg += tr("INCLUDE must be placed in a separate line");
				break;
			case COMPERR_INCLUDENOFILE:
				msg += tr("No file specified for INCLUDE");
				break;
			case COMPERR_ONERRORCALL:
				msg += tr("Cannot pass arguments to a SUBROUTINE used by ONERROR statement");
				break;
			case COMPERR_NUMBERTOOLARGE:
				msg += tr("Number too large");
				break;
			case COMPERR_SORTOPTION:
				msg += tr("SORT option repeated, or both ASCENDING and DESCENDING given");
				break;

			default:
				if(column==0) {
					msg += tr("Syntax error around beginning line");
				} else {
					msg += tr("Syntax error around character ") + QString::number(column);
				}
		}
		msg += QStringLiteral(".\n");
		emit(outputError(msg));

		freeBasicParse();

		status = R_STOPPED;
		return -1;
	}

	currentLine = 1;
	return 0;
}

void
Interpreter::initialize() {
	error->loadSettings();
	imageSmooth = false;
	op = wordCode;
	callstack = new addrStack();
	onerrorstack = new addrStack();
	onstopaddr = NULL;
	trycatchstack = NULL;
	forstack = NULL;
	forstacklevelsize = 0;
	status = R_RUNNING;
	//initialize random
	double_random_max = (double) RAND_MAX * (double) RAND_MAX + (double) RAND_MAX + 1.0;
	currentLine = 1;
	includeFileNumber = 0;
	emit(resizeGraphWindow(GSIZE_INITIAL_WIDTH, GSIZE_INITIAL_HEIGHT, 1.0));

	painter = new QPainter();
	painter_custom_font_flag = false;
	setGraph(""); //after resizeGraphWindow()
	defaultfontfamily = painter->font().family();
	defaultfontpointsize = painter->font().pointSize();
	defaultfontweight = painter->font().weight();
	defaultfontitalic = painter->font().italic();
	drawingpen = QPen(Qt::black); // default pen color
	drawingbrush = QBrush(Qt::black, Qt::SolidPattern); // default brush color
	painter_pen_color = Qt::black; //last color
	painter_brush_color = Qt::black; //last color
	CompositionModeClear = false;
	PenColorIsClear = false;
	fastgraphics = false;
	frameRateSet = false;
	// drop a stop signal left standing by the previous run so it cannot
	// shorten this run's first PAUSE
	sleeper->clearWake();
	windowActive = false;
	winX1 = winY1 = winX2 = winY2 = 0.0;
	windowTransform.reset();
	windowInverse.reset();

	sprites.startRun();
	printing = false;
	regexMinimal = false;
	basicKeyboard->reset();
	// clickclear mouse status
	graphics->clickX = 0;
	graphics->clickY = 0;
	graphics->clickB = 0;

	// create the convert and comparer object
	convert = new Convert(locale);

	// now build the new stack object
	stack = new Stack(convert);
	savestack = new Stack(convert);		// secondary stack to hold stuff (OP_STACKSAVE, OP_STACKUNSAVE)
	
	// now create the variable storage and set arraybase
	variables = new Variables(numsyms);
	arraybase = 0;


	// initialize pointers used for database recordsets (querries)
	for (int t=0; t<NUMDBCONN; t++) {
		for (int u=0; u<NUMDBSET; u++) {
			dbSet[t][u] = NULL;
		}
	}

	// initialize files to NULL (closed)
	filehandle = new QIODevice*[NUMFILES];
	filehandletype = new int[NUMFILES];
	for (int t=0; t<NUMFILES; t++) {
    	filehandle[t] = nullptr;
    	filehandletype[t] = 0;
	}
	
	// save IDE path so that it can be restored after program terminates
	originalPath = QDir::currentPath();
}


// MediaLoader's way of handing a sound's bytes to the sound system, which
// lives on the main thread: a blocking round trip, like every other signal
// the interpreter waits on.
void
Interpreter::registerSoundBytes(const QString &id, QByteArray *bytes) {
	mymutex->lock();
	emit(loadSoundFromArray(id, bytes));
	waitCond->wait(mymutex);
	mymutex->unlock();
}


// FileSecurity's way of asking about a path outside the program's folder.
// The interpreter is the one place that talks to the GUI, so the question is
// put from here; the answer is a SETTINGSALLOW* value.
int
Interpreter::askAllowFile(const QString &what, const QString &resolved) {
	if (guiState == GUISTATESILENT) {
		// --silent: nobody to ask, so fail closed
		return SETTINGSALLOWNO;
	}
	mymutex->lock();
	emit(dialogAllowFile(what, resolved));
	waitCond->wait(mymutex);
	mymutex->unlock();
	return returnInt;
}


void
Interpreter::cleanup() {
	// cleanup that MUST happen for run to early terminate is in runHalted
	// called by run() once the run is terminated
	//
	// Clean up run time objects

#ifdef BASIC256_ENABLE_PROCESS
	// sys is only ever non-null when this flag is on (see OP_SYSTEM) --
	// Qt for WebAssembly's QProcess doesn't implement kill() at all (process
	// spawning is meaningless in a browser sandbox), so this must not even
	// compile when the flag -- and thus WASM -- is off.
	if(sys) sys->kill();
#endif

	// stop timers
	sleeper->wake();

	// stop downloading
	if(downloader!=NULL) downloader->stop();
	delete (downloader);
	downloader = NULL;
	
	// stop playing any sound
	if(sound!=NULL) sound->exit();

	// close network connections
	netSockCloseAll();

	// delete the stack
	delete(stack);
	delete(savestack);

	//delete stack used by nested for statements
	int level = variables->getrecurse();
	forframe *temp_forstack;
	while(level>=0){
		while (forstack!=NULL) {
			temp_forstack = forstack;
			forstack = temp_forstack->next;
			delete(temp_forstack);
		}
		level--;
		if(level>0) forstack=forstacklevel[level];
	}

	delete(convert);
	// Clean up sprites
	sprites.clear();
	
	// Clean up, for frames, etc.
	freeBasicParse();
	
	// close open files (set to NULL if closed)
	for (int t=0; t<NUMFILES; t++) {
		if (filehandle[t]) {
			filehandle[t]->close();
			filehandle[t] = NULL;
			filehandletype[t] = 0;
		}
	}
	delete[] filehandle;
	delete[] filehandletype;

	// close open database connections and record sets
	for (int t=0; t<NUMDBCONN; t++) {
		closeDatabase(t);
	}

	// close and delete painter
	if (painter) {
		if(painter->isActive()) painter->end();
		delete painter;
		painter=NULL;
	}
#ifdef BASIC256_ENABLE_PRINTER
	if(printing){
		printdocument->abort(); //try to abort printing
		delete printdocument;
	}
#endif

	// close network connections
	netSockCloseAll();

	// remove any queued errors
	error->deq();

	//delete stack used by function calls, subroutine calls, and gosubs for return location
	delete callstack;

	//delete stack used to track nested on-error definitions
	delete onerrorstack;

	//delete stack used to track nested try/catch definitions
	trycatchframe *temp_trycatchstack;
	while (trycatchstack!=NULL) {
		temp_trycatchstack = trycatchstack;
		trycatchstack = temp_trycatchstack->next;
		delete(temp_trycatchstack);
	}

	//clear images
	QMap<QString, QImage*>::const_iterator it = images.constBegin();
	while (it != images.constEnd()) {
		delete(it.value());
		++it;
	}
	images.clear();
	
	// clear variables, maps, and arrays
	//fprintf(stderr,"interperter b4 delete variables\n");
	delete variables;

	// restore IDE path
	QDir::setCurrent(originalPath);
}

void Interpreter::closeDatabase(int t) {
#ifdef BASIC256_ENABLE_SQL
	// cleanup database and all of its sets
	QString dbconnection = QStringLiteral("DBCONNECTION") + QString::number(t);
	QSqlDatabase db = QSqlDatabase::database(dbconnection);
	if (db.isValid()) {
		for (int u=0; u<NUMDBSET; u++) {
			if (dbSet[t][u]) {
				dbSet[t][u]->clear();
				delete dbSet[t][u];
				dbSet[t][u] = NULL;
			}
		}
		db.close();
		db = QSqlDatabase();
		QSqlDatabase::removeDatabase(dbconnection);
	}
#else
	(void)t;
#endif
}

void
Interpreter::runHalted() {
	// event fires from runcoltroller to tell program to signal user stop
	// force the interperter ops that block to go ahead and quit
}


void
Interpreter::run() {
	//read important settings from start
	SETTINGS;
	settingsDebugSpeed = settings.value(SETTINGSDEBUGSPEED, SETTINGSDEBUGSPEEDDEFAULT).toInt();
	settingsAllowSystem = settings.value(SETTINGSALLOWSYSTEM, SETTINGSALLOWSYSTEMDEFAULT).toInt();
	settingsAllowSetting = settings.value(SETTINGSALLOWSETTING, SETTINGSALLOWSETTINGDEFAULT).toBool();
	settingsAllowPort = settings.value(SETTINGSALLOWPORT, SETTINGSALLOWPORTDEFAULT).toInt();
	settingsNetListenAny = settings.value(SETTINGSNETLISTENANY, SETTINGSNETLISTENANYDEFAULT).toBool();
	// the file rule is anchored to the folder the program was loaded from
	fileSecurity.startRun(QDir::currentPath(), settings.value(SETTINGSALLOWFILE, SETTINGSALLOWFILEDEFAULT).toInt());
	settingsSettingsAccess = settings.value(SETTINGSSETTINGSACCESS, SETTINGSSETTINGSACCESSDEFAULT).toInt();
	settingsSettingsMax = settings.value(SETTINGSSETTINGSMAX, SETTINGSSETTINGSMAXDEFAULT).toInt();
	programName.clear();

	settingsPrinterResolution = settings.value(SETTINGSPRINTERRESOLUTION, SETTINGSPRINTERRESOLUTIONDEFAULT).toInt();
	settingsPrinterPrinter = settings.value(SETTINGSPRINTERPRINTER, 0).toInt();
	settingsPrinterPaper = settings.value(SETTINGSPRINTERPAPER, SETTINGSPRINTERPAPERDEFAULT).toInt();
	settingsPrinterPdfFile = settings.value(SETTINGSPRINTERPDFFILE, "./output.pdf").toString();
	settingsPrinterOrient = settings.value(SETTINGSPRINTERORIENT, SETTINGSPRINTERORIENTDEFAULT).toInt();

	// main run loop
	isError=false;
	downloader = new BasicDownloader(error);
	media.startRun();
	mediaplayer_id_legacy = 0;
	//link sound system to error mechanism
	sound->error = &error;
	srand(time(NULL)+QTime::currentTime().msec()*911L); rand(); rand(); 	// initialize the random number generator for this thread
	noiseSeed = (int64_t)time(NULL) ^ ((int64_t)QTime::currentTime().msec() * 911L);	// and the noise field, so an unseeded run differs like RAND does
	runtimer.start(); // used by MSEC function
	runLoop();			// run the opcodes
	debugMode = 0;
	//qDebug() << "run - before cleanup()";
	cleanup(); // cleanup the variables, databases, files, stack and everything
	emit(stopRunFinalized(!isError));
}

void Interpreter::runLoop() {
	// execByteCode() now runs opcodes until the program stops or a fatal error
	// is raised - it returns 0 when it saw R_STOPPING and -1 when it died
	int rv = execByteCode();
	if (status == R_STOPPING && onstopaddr && rv >=0 ) {
		// is there an onstop handler for user event stop
		// and we are not at a programatic "end"
		//qDebug() << "onstop" << op;
		callstack->push(op);
		op = onstopaddr;
		status = R_RUNNING;
		runLoop();
	}
}

void Interpreter::waitForGraphics() {
	// --silent: nothing is ever shown, so skip sprite compositing and the
	// cross-thread screen-image update entirely rather than doing that work
	// against a window nobody will ever see.
	if (guiState == GUISTATESILENT) return;
	sprites.updateScreen();
	// wait for graphics operation to complete
	mymutex->lock();
	emit(goutputReady());
	waitCond->wait(mymutex);
	mymutex->unlock();
}

// Build the window-units -> surface-pixels map for a surface w x h. A window
// runs from (winX1,winY1) at the surface's top-left corner to (winX2,winY2) at
// its bottom-right, so the sign of winX2-winX1 and winY2-winY1 chooses which
// way each axis runs: WINDOW -1,-1,1,1 puts y=-1 at the top (screen order) and
// WINDOW -1,1,1,-1 puts y=1 at the top (maths order).
void Interpreter::updateWindowTransform(int w, int h) {
	if (!windowActive || w <= 0 || h <= 0) {
		windowTransform.reset();
		windowInverse.reset();
		return;
	}
	double sx = (double) w / (winX2 - winX1);
	double sy = (double) h / (winY2 - winY1);
	// QTransform(m11, m12, m21, m22, dx, dy) maps
	//   x' = m11*x + m21*y + dx,  y' = m12*x + m22*y + dy
	windowTransform = QTransform(sx, 0.0, 0.0, sy, -sx * winX1, -sy * winY1);
	windowInverse = windowTransform.inverted();
}

bool Interpreter::setPainterTo(QPaintDevice *destination) {
	drawingOnScreen = (destination == graphics->image);
	if(painter->isActive()) painter->end();
	painter_pen_need_update=true;
	painter_brush_need_update=true;
	painter_font_need_update=painter_custom_font_flag; //need update only if there is a custom font loaded
	painter_last_compositionModeClear=false;
	if (guiState == GUISTATESILENT && drawingOnScreen) {
		// --silent: never paint onto the (never shown) on-screen canvas. Leaving
		// painter inactive makes every subsequent painter->drawXXX()/setXXX() call
		// a documented Qt no-op, so plot/circle/rect/etc. are skipped for free
		// without needing to touch every opcode. Drawing to an in-memory "image:"
		// resource (drawingOnScreen==false) is unaffected and still works normally.
		return true;
	}
	if (!painter->begin(destination)) return false;
	// The window maps onto whichever surface we just started painting on, so
	// WINDOW composes with SETGRAPH: the same window spans a 200x200 image
	// resource and the full graphics canvas alike.
	updateWindowTransform(destination->width(), destination->height());
	if (windowActive) {
		painter->setWorldTransform(windowTransform);
		// PENWIDTH and FONT stay in surface pixels, so a line does not grow
		// thicker just because the window makes a unit large. A cosmetic pen is
		// measured in device space whatever the world transform says.
		drawingpen.setCosmetic(true);
		painter_pen_need_update = true;
	} else if (drawingpen.isCosmetic()) {
		drawingpen.setCosmetic(false);
		painter_pen_need_update = true;
	}
	return true;
}

void Interpreter::setGraph(QString id){
	if(id.isEmpty()){
		setPainterTo(graphics->image);
		drawto = QString("");
	} else if(id.startsWith("image:")){
		if (images.contains(id)){
			setPainterTo(images[id]);
			drawto = id;
		}else{
			setPainterTo(graphics->image);
			drawto = QString("");
			error->q(ERROR_IMAGERESOURCE);
		}
	}else{
		error->q(ERROR_INVALIDRESOURCE);
	}
}


int
Interpreter::execByteCode() {
	int opcode;

	// The interpreter loop lives here rather than in runLoop() so that the
	// prologue and epilogue of this very large function run once per run
	// instead of once per opcode.  Finishing an opcode used to be "return 0"
	// and a fresh call from runLoop() - it is now a jump back to nextop.
	// A jump rather than a loop with "continue" because the opcode bodies are
	// full of nested loops and switches that "continue" would bind to instead.
nextop:
	if (status == R_STOPPING) return 0;

	// if errnum is set then handle the last thrown error
	if (error->pending()) {
		//change ERROR_VARNOTASSIGNED into ERROR_ARRAYELEMENT if variable implied is in fact an array element
		//still need this for functions like implode
		if(error->pending_e==ERROR_VARNOTASSIGNED && error->pending_var>=0){
			DataElement *e = variables->getData(error->pending_var);
			if(DataElement::getType(e)==T_ARRAY) error->pending_e=ERROR_ARRAYELEMENT;
		}
		if(error->pending_e==WARNING_VARNOTASSIGNED && error->pending_var>=0){
			DataElement *e = variables->getData(error->pending_var);
			if(DataElement::getType(e)==T_ARRAY) error->pending_e=WARNING_ARRAYELEMENT;
		}

		error->process(currentLine);

		if(trycatchstack) {
			// remove try/catch trap and jump to the catch label
			op = trycatchstack->catchAddr;
			//go back to the original recurse level of the try/catch trap
			while(trycatchstack->recurseLevel<variables->getrecurse()){
				decreaserecurse();
			}
			//clear the stack to original size
			if(stack->height()>trycatchstack->stackSize){
				stack->drop(stack->height()-trycatchstack->stackSize);
			}
			//delete trap
			trycatchframe *temp_trycatchstack = trycatchstack;
			trycatchstack=trycatchstack->next;
			delete temp_trycatchstack;
			goto nextop;
		}else if(onerrorstack->count() > 0) {
			//there is on-error defined
			// progess call to subroutine for error handling
			callstack->push(op);
			op = onerrorstack->peek();
			goto nextop;
		} else {
			isError=true;
			// no error handler defined or FATAL error - display message
			printError();
			// if error number less than the start of warnings then
			// highlight the current line AND die
			if (error->isFatal()) {
				if (includeFileNumber==0) emit(goToLine(error->line));
				return -1;
			}
		}
	}

	#ifdef DEBUG 
	{
    // displays the opcode - its argument and the stack (Before) execution
    qDebug() << "stackbefore -" << stack->debug();
    qDebug() << QString("%1 %2")
                .arg((unsigned int)(op - wordCode), 8, 16, QChar('0'))
                .arg(opname(*op));

    if (optype(*op) == OPTYPE_INT) {
        if ((*op) == OP_CURRLINE) {
            int includeFileNumber = (long) *(op+1) >> 24;
            int currentLine       = (long) *(op+1) & 0xffffff;
            qDebug() << includeFileNumber << currentLine;
        } else {
            qDebug() << (long) *(op+1);
        }
    }
    if (optype(*op) == OPTYPE_FLOAT)
        qDebug() << (float) *(op+1);


    if (optype(*op) == OPTYPE_STRING)
        qDebug() << QString::fromUtf8((char *)(op+1));

    if (optype(*op) == OPTYPE_LABEL) {
        int v = *(op+1);
        qDebug() << "lbl" << ((v >= 0 && v < numsyms) ? symtable[v] : "__unknown__");
    }
    if (optype(*op) == OPTYPE_VARIABLE) {
        int v = *(op+1);
        qDebug() << ((v >= 0 && v < numsyms) ? symtable[v] : "__unknown__");
    }
    if (optype(*op) == OPTYPE_VAR_VAR) {
        int v  = *(op+1);
        int v2 = *(op+2);
        qDebug() << ((v  >= 0 && v  < numsyms) ? symtable[v]  : "__unknown__")
                 << ((v2 >= 0 && v2 < numsyms) ? symtable[v2] : "__none__");
    }
	if (optype(*op) == OPTYPE_LONG) {
    	qDebug() << *(qint64*)(op+1);
	}	
    qDebug() << "---"; // replaces the bare fprintf(stderr, "\n") line separator
}
#endif

	opcode = *op;
	op++;

	switch (optypeindex(opcode)) {
		case optypeindex(OPTYPE_VAR_VAR): {
			//
			// OPCODES with two variable numbers following in the wordCcode go in this switch
			// int i is the symbol/variable number
			// int i2 is the second symbol/variable number
			//
			int i = *op;
			op++;
			int i2 = *op;
			op++;

			switch(opcode) {

				case OP_FOREACH: {
					int *nextAddr = wordCode + stack->popInt();
					DataElement *d = stack->popDE();			// RELEASE - released when loop terminates next/ exitfor
					
					// two variables
					// - i is the variable for the value in an array and the key for a map
					// - i2 is the variable for the value in a map
					

					// search for previous FOR in stack
					// this is done because of anti-spaghetti code

					forframe *temp = forstack;
					forframe *prev = NULL;
					while(temp && nextAddr!=temp->nextAddr){
						prev=temp;
						temp=temp->next;
					}
					if(temp){
						//if there is a previous FOR in stack pull it to use the allocated memory
						if(prev){
							//node is in the middle of stack
							prev->next=temp->next;
						}else{
							//node is last
							forstack=temp->next;
						}
					}else{
						//or create a new node
						temp = new forframe;
					}

					switch(DataElement::getType(d)) {
						case T_ARRAY:
						{
							if(d->arrayCols()>=1) {
								// create forframe
								temp->forVarnum = i;
								temp->forVarnumValue = i2;
								temp->forFrameType = FORFRAMETYPE_FOREACH_ARRAY;
								temp->foreach_de = d;
								temp->arrayIter = d->arr->data.begin();
								temp->arrayIterEnd = d->arr->data.end();
								// set variable to first element
								variables->setData(temp->forVarnum, &*temp->arrayIter);
								watchvariable(debugMode, temp->forVarnum);
								// add new forframe to the forframe stack
								temp->next = forstack;
								temp->forAddr = op;		// first op after FOR statement to loop back to
								temp->nextAddr = nextAddr;
								forstack = temp;
							} else {
								// no data in array - jump to next
								delete d;
								delete temp;
								op = nextAddr;
							}
						}
						break;
						case T_MAP:
						{
#ifdef DEBUG
fprintf(stderr,"in foreach map %d\n", d->map->data.size());
#endif
							if(d->map->data.size()>=1) {
								// create forframe
								temp->forVarnum = i;
								temp->forVarnumValue = i2;
								temp->forFrameType = FORFRAMETYPE_FOREACH_MAP;
								temp->foreach_de = d;
								temp->mapIter = d->map->data.begin();
								temp->mapIterEnd = d->map->data.end();
								// set variable1 to first key
								variables->setData(temp->forVarnum, QString::fromStdString((std::string) (temp->mapIter->first)));
								watchvariable(debugMode, temp->forVarnum);
								// set variable2 to first value
								if (temp->forVarnumValue !=-1) {
									variables->setData(temp->forVarnumValue, temp->mapIter->second);
									watchvariable(debugMode, temp->forVarnumValue);
								}
								// add new forframe to the forframe stack
								temp->next = forstack;
								temp->forAddr = op;		// first op after FOR statement to loop back to
								temp->nextAddr = nextAddr;
								forstack = temp;
							} else {
								// no data in array - jump to next
								delete d;
								delete temp;
								op = nextAddr;
							}
						}
						break;
						default:
						{
							error->q(ERROR_ARRAYORMAPEXPR);
							delete d;
							delete temp;
						}
					}
				}
				break;

				// add additional optype var_var here

			}
			break;
		}	

		case optypeindex(OPTYPE_VARIABLE): {
			//
			// OPCODES with an variable number following in the wordCcode go in this switch
			// int i is the symbol/variable number
			//
			int i = *op;
			op++;

			switch(opcode) {

				case OP_FOR: {
					int *nextAddr = wordCode + stack->popInt();
					DataElement *stepE = stack->popDE();			// RELEASE
					DataElement *endE = stack->popDE();			// RELEASE
					DataElement *startE = stack->popDE();			// RELEASE

					// search for previous FOR in stack
					// this is done because of anti-spaghetti code

					forframe *temp = forstack;
					forframe *prev = NULL;
					while(temp && nextAddr!=temp->nextAddr){
						prev=temp;
						temp=temp->next;
					}
					if(temp){
						//if there is a previous FOR in stack pull it to use the allocated memory
						if(prev){
							//node is in the middle of stack
							prev->next=temp->next;
						}else{
							//node is last
							forstack=temp->next;
						}
					}else{
						//or create a new node
						temp = new forframe;
					}

					bool goodloop;	// set to true if we should do the loop atleast once

					variables->setData(i, startE);	// set variable to initial value
					watchvariable(debugMode, i);

					if (DataElement::getType(startE)==T_INT && DataElement::getType(stepE)==T_INT) {
						// an integer start and step (do an integer loop)
						temp->forFrameType = FORFRAMETYPE_INT;
						temp->forVarnum = i;
						temp->forVarnumValue = -1;
						temp->intStart = startE->intval;
						temp->intEnd = convert->getLong(endE);	// could b float but cant ever be
						temp->intStep = stepE->intval;
						goodloop = (temp->intStep>0 && temp->intStart <= temp->intEnd) ||
							(temp->intStep<0 && temp->intStart >= temp->intEnd) ||
							(temp->intStep==0);
					} else {
						// start or step not integer - it is a float loop
						temp->forFrameType = FORFRAMETYPE_FLOAT;
						temp->forVarnum = i;
						temp->forVarnumValue = -1;
						temp->floatStart = convert->getFloat(startE);
						temp->floatEnd = convert->getFloat(endE);
						temp->floatStep = convert->getFloat(stepE);
						goodloop = (temp->floatStep > 0.0 && temp->floatStart <= temp->floatEnd) ||
							(temp->floatStep < 0.0 && temp->floatStart >= temp->floatEnd) ||
							(temp->floatStep == 0.0);
					}

					if (goodloop) {
						// add new forframe to the forframe stack
						temp->next = forstack;
						temp->forAddr = op;		// first op after FOR statement to loop back to
						temp->nextAddr = nextAddr;
						forstack = temp;
					} else {
						// bad loop exit straight away - jump to the statement after the next
						delete temp;
						op = nextAddr;
					}
					delete stepE;
					delete startE;
					delete endE;
				}
				break;


				case OP_DIM:
				case OP_REDIM: {
					int y = stack->popInt();
					int x = stack->popInt();
					if (x<=0) x=1; // need to dimension as 1d
					variables->getData(i)->arrayDim(x, y, opcode == OP_REDIM);
					if (DataElement::getError()) {error->q(DataElement::getError(true),i,x,y);}
					watchvariable(debugMode, i);
				}
				break;
				
				case OP_MAP_DIM:
				{
					variables->getData(i)->mapDim();
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					watchvariable(debugMode, i);
				}
				break;
				

				case OP_ALEN: {
					long l = 0;
					DataElement *e = variables->getData(i);			// DONT RELEASE
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					switch (DataElement::getType(e)) {
						case T_ARRAY:
						{
							if (e->arrayRows()==1) {
								l = e->arrayCols();
							} else {
								error->q(ERROR_ARRAYLENGTH2D);
							}
						}
						break;
						case T_MAP:
						{
							l = e->mapLength();
						}
						break;
						default:
							error->q(ERROR_ARRAYORMAPEXPR);
					}
					stack->pushInt(l);
				}
				break;

				case OP_ALENROWS: {
					long l = 0;
					DataElement *e = variables->getData(i);			// DONT RELEASE
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					l = e->arrayRows();
					stack->pushInt(l);
				}
				break;

				case OP_ALENCOLS: {
					long l = 0;
					DataElement *e = variables->getData(i);			// DONT RELEASE
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					l = e->arrayCols();
					stack->pushInt(l);
				}
				break;

				case OP_GLOBAL: {
					// make a variable number a global variable
					variables->makeglobal(i);
				}
				break;

				case OP_ARR_SET: {
					// assign a value to an array element
					// assumes that arrays are always two dimensional (if 1d then one row [0,i]) )
					// borrowed: the stack keeps these, and nothing here pushes
					// before the last read of them
					DataElement *e = stack->popDEborrow();			// DO NOT RELEASE
					DataElement *col = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *row = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *vdata = variables->getData(i);			// DONT RELEASE
					switch (DataElement::getType(vdata)) {
						case T_ARRAY:
						{
							int c = convert->getInt(col) - arraybase;
							int r = convert->getInt(row) - arraybase;
							if (c<0) c=0;
							if (r<0) r=0;
							vdata->arraySetData(r, c, e);
							if (DataElement::getError()) {error->q(DataElement::getError(true),i,r+arraybase,c+arraybase);}
							watchvariable(debugMode, i, r, c);
						}
						break;
						case T_MAP:
						{
							QString key = convert->getString(col);
							vdata->mapSetData(key, e);
							if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
							watchvariable(debugMode, i, key);
						}
						break;
						default:
							error->q(ERROR_ARRAYORMAPEXPR);
					}
				}
				break;


				case OP_ARR_GET: {
					// get a value from an array and push it to the stack
					// borrowed: both are read into r and c, and into the map key,
					// before anything is pushed over them
					DataElement *col = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *row = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *vdata = variables->getData(i);			// DONT RELEASE
					switch (DataElement::getType(vdata)) {
						case T_ARRAY:
						{
							int c = convert->getInt(col) - arraybase;
							int r = convert->getInt(row) - arraybase;
							if (c<0) c=0;
							if (r<0) r=0;
							DataElement *e;
							e = vdata->arrayGetData(r, c);		// DONT RELEASE
							if (DataElement::getError()) {error->q(DataElement::getError(true),i,r+arraybase,c+arraybase);}
							stack->pushDE(e);
						}
						break;
						case T_MAP:
						{
							QString key = convert->getString(col);
							DataElement *e;
							e = vdata->mapGetData(key);			// DONT RELEASE
							if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
							stack->pushDE(e);
						}
						break;
						default:
							error->q(ERROR_ARRAYORMAPEXPR);
							stack->pushBool(false);
					}
				}
				break;

				case OP_VAR_GET: {
					DataElement *e = variables->getData(i);			// DONT RELEASE
					if (DataElement::getType(e)==T_UNASSIGNED) {
						error->q(ERROR_VARNOTASSIGNED,i);
						stack->pushUnassigned();
					} else {
						stack->pushDE(e);
					}
				}
				break;

				case OP_VAR_REF: {
					stack->pushRef(i, variables->getrecurse());
				}
				break;

				case OP_VAR_SET: {
					// assign a value to a variable
					DataElement *e = stack->popDEborrow();		// DO NOT RELEASE
					variables->setData(i,e);
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					watchvariable(debugMode, i);
				}
				break;

				case OP_ARRAY2STACK: {
					// Push all of the elements of an array to the stack and then push the length to the stack
					// expects one integer - variable number
					// all arrays are 2 dimensional - push each column, column size, then number of rows
					DataElement *e = variables->getData(i);			// DONT RELEASE
					int columns = e->arrayCols();
					int rows = e->arrayRows();
					if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
					if(!error->pending()){
						for(int row = 0; row<rows; row++) {
							for (int col = 0; col<columns; col++) {
								DataElement *d = e->arrayGetData(row, col);			// DONT RELEASE
								if (DataElement::getType(d)==T_UNASSIGNED) {
									error->q(ERROR_VARNOTASSIGNED, i, row, col);
									stack->pushUnassigned();
								} else {
									stack->pushDE(d);
								}
							}
							stack->pushInt(columns);
						}
						stack->pushInt(rows);
					}else{
						//0 rows, 0 columns if error
						stack->pushInt(0);
						stack->pushInt(0);
					}
				}
				break;

				case OP_ARRAYFILL: {
					// fill an array with a single value
					int mode = stack->popInt();		// 1-fill everything, 0-fill unassigned
					DataElement *e = stack->popDE();	// fill value			// RELEASE
					if (DataElement::getType(e)==T_UNASSIGNED) {
						error->q(ERROR_VARNOTASSIGNED);
					} else if (DataElement::getType(e)==T_ARRAY) {
						error->q(ERROR_ARRAYINDEXMISSING);
					} else {
						DataElement *edest = variables->getData(i);			// DONT RELEASE
						if (DataElement::getType(edest)==T_ARRAY) {
							int columns = edest->arrayCols();
							int rows = edest->arrayRows();
							if (!error->pending() && DataElement::getType(edest)==T_ARRAY) {
								// Pushes nothing. It used to push `columns` once per row and
								// then `rows`, but OP_ARRAYFILL is only ever emitted at
								// statement level (dimstmt/redimstmt/arrayassign) and nothing
								// popped them -- so every "... fill v" leaked rows+1 ints onto
								// a stack that grows on demand. Harmless once; unbounded for a
								// fill inside a loop, which a zero-filling bare DIM now makes
								// commonplace.
								// Mode 1 overwrites everything, so it has no reason to read
								// the element first.  It used to read one anyway, which set
								// DataElement's not-assigned flag for every element of a
								// fresh array; nothing consumed it, and it only stayed
								// invisible because allocating the replacement element ran
								// a constructor, and the constructor clears that flag.
								// Elements are no longer allocated one at a time, so the
								// read has to go - and mode 0, which does need to know
								// whether the element is set, clears the flag it raises.
								for(int row = 0; row<rows; row++) {
									for (int col = 0; col<columns; col++) {
										if (mode) {
											edest->arraySetData(row, col, e);
										} else {
											DataElement *temp = edest->arrayGetData(row, col);			// DONT RELEASE
											const bool unassigned = (DataElement::getType(temp)==T_UNASSIGNED);
											if (unassigned) DataElement::getError(true);	// "not assigned" is the answer here, not an error
											if (unassigned) edest->arraySetData(row, col, e);
										}
									}
								}
							}
						 } else {
							// trying to fill a regular variable - just do an assign
							variables->setData(i, e);
						}
						watchvariable(debugMode, i);
					}
					delete e;
				}
				break;

				case OP_ARRAYLISTASSIGN: {
					const int rows = stack->popInt();
					const int columns = stack->popInt(); //pop the first row length - the following rows must have the same length
					int columns2 = columns;
					DataElement *edest = variables->getData(i);			// DONT RELEASE
					
					// create array if we need to (wrong dimensions or not array)
					if (DataElement::getType(edest) != T_ARRAY || edest->arrayCols()!=columns || edest->arrayRows()!=rows) {
						edest->arrayDim(rows, columns, false);
					}

					for(int row = rows-1; row>=0; row--) {
						//pop row length only if is not first row - already popped
						if(row != rows-1) {
							columns2=stack->popInt();
							if(columns2!=columns){
								error->q(ERROR_ARRAYNITEMS, i);
								// empty stack to successfully pass over an OnError situation
								stack->drop(columns2);
								for(row--; row>=0 ; row--) stack->drop(stack->popInt());
								break;
							}
						}
						for(int col= columns-1; col >= 0; col--) {
							//continue to pull from stack even if error occurred
							DataElement *e = stack->popDE();			// RELEASE
							edest->arraySetData(row, col, e);
							delete e;
						}
					}
					watchvariable(debugMode, i);
				}
				break;

				case OP_VAR_ASSIGNED: {
					DataElement *e = variables->getData(i);			// DONT RELEASE
					stack->pushBool(DataElement::getType(e)!=T_UNASSIGNED);
				}
				break;

				case OP_ARR_ASSIGNED: {
					// clear a variable and release resources
					DataElement *col = stack->popDE();			// RELEASE
					DataElement *row = stack->popDE();			// RELEASE
					DataElement *vdata = variables->getData(i);			// DONT RELEASE
					DataElement *e;
					switch (DataElement::getType(vdata)) {
						case T_ARRAY:
						{
							int c = convert->getInt(col) - arraybase;
							int r = convert->getInt(row) - arraybase;
							e = vdata->arrayGetData(r, c);			// DONT RELEASE
							stack->pushBool(DataElement::getType(e)!=T_UNASSIGNED);
							if (DataElement::getError()) {error->q(DataElement::getError(true),i,r+arraybase,c+arraybase);}
						}
						break;
						case T_MAP:
						{
							QString key = convert->getString(col);
							e = vdata->mapGetData(key);			// DONT RELEASE
							stack->pushBool(DataElement::getType(e)!=T_UNASSIGNED);
							if (DataElement::getError()) {error->q(DataElement::getError(true),i);}
						}
						break;
						default:
						{
							error->q(ERROR_ARRAYORMAPEXPR);
							stack->pushBool(false);
						}
					}
					delete col;
					delete row;
				}
				break;

				case OP_VAR_UN: {
					variables->unassign(i);
					watchvariable(debugMode, i);
				}
				break;

				case OP_MATADD:
				case OP_MATSUB:
				case OP_MATMUL: {
					// MAT ADD/SUB/MUL destination = source op operand.  The stack
					// holds a reference to the source matrix and then the operand,
					// which is a reference too when it was written as a plain
					// variable name - see matRightOperand() in basicParse.y - and
					// an ordinary value otherwise.  Only the run time can tell
					// whether a name holds a matrix or a single number.
					DataElement *right = stack->popDE();			// RELEASE
					DataElement *left = stack->popDE();			// RELEASE
					int leftvar = -1;
					int rightvar = -1;
					DataElement *leftdata = left;
					DataElement *rightdata = right;
					if (DataElement::getType(left)==T_REF) {
						leftvar = left->intval;
						leftdata = variables->get(left->intval, left->level)->data;		// DONT RELEASE
					}
					if (DataElement::getType(right)==T_REF) {
						rightvar = right->intval;
						rightdata = variables->get(right->intval, right->level)->data;	// DONT RELEASE
					}
					matStatement(opcode, i, leftdata, leftvar, rightdata, rightvar);
					watchvariable(debugMode, i);
					delete left;
					delete right;
				}
				break;

				case OP_MATTRN:
				case OP_MATINV: {
					// MAT TRN/INV destination = source - one operand, always a
					// reference to the source matrix
					DataElement *left = stack->popDE();			// RELEASE
					int leftvar = -1;
					DataElement *leftdata = left;
					if (DataElement::getType(left)==T_REF) {
						leftvar = left->intval;
						leftdata = variables->get(left->intval, left->level)->data;		// DONT RELEASE
					}
					matStatement(opcode, i, leftdata, leftvar, NULL, -1);
					watchvariable(debugMode, i);
					delete left;
				}
				break;

				// Arrays: the work is done in Interpreter_arrays.cpp
				case OP_SORT:
				case OP_ARRAYADD:
					execArrayOp(opcode, i);
				break;

				case OP_ARR_UN: {
					// clear a variable and release resources
					DataElement *col = stack->popDE();			// RELEASE
					DataElement *row = stack->popDE();			// RELEASE
					DataElement *vdata = variables->getData(i);			// DONT RELEASE
					switch (DataElement::getType(vdata)) {
						case T_ARRAY:
						{
							int c = convert->getInt(col) - arraybase;
							int r = convert->getInt(row) - arraybase;
							vdata->arrayUnassign(r, c);
							watchvariable(debugMode, i, r, c);
						}
						break;
						case T_MAP:
						{
							QString key = convert->getString(col);
							vdata->mapUnassign(key);
							watchvariable(debugMode, i, key);
						}
						break;
						default:
						{
							error->q(ERROR_ARRAYORMAPEXPR);
						}
					}
					delete col;
					delete row;
				}
				break;

				case OP_VARIABLEWATCH: {
					watchvariable(true, i);
				}
				break;

				// additional variable number ops added here

			}
		}
		break;

		case optypeindex(OPTYPE_INT): {
			//
			// OPCODES with an integer following in the wordCcode go in this switch
			// int i is the number extracted from the wordCode
			//
			int i = *op;  // integer for opcodes of this type
			op++;
			switch(opcode) {

				case OP_CURRLINE: {
					// opcode currentline is compound and includes file number (ID) and line number
					//includeFileNumber = i / 0x1000000;
					//currentLine = i % 0x1000000;
					includeFileNumber = i >> 24;
					currentLine = i & 0xffffff;

					if (debugMode != 0) {
						if (includeFileNumber==0) {
							// do debug for the main program not included parts
							// edit needs to eventually have tabs for includes and tracing and debugging
							// would go three dimensional - but not right now
							emit(seekLine(currentLine));
							if ((debugMode==1) || (debugMode==2 && debugBreakPoints->contains(currentLine-1))) {
								// show step and runto options
								mydebugmutex->lock();
								emit(debugNextStep());
								// wait for button if we are stepping or if we are at a break point
								waitDebugCond->wait(mydebugmutex);
								mydebugmutex->unlock();
							} else {
								// when debugging to breakpoint slow execution down so that the
								// trace on the screen keeps caught up
								sleeper->sleepMS(settingsDebugSpeed);
							}
						}
					}
				}
				break;

				case OP_PUSHINT: {
					stack->pushInt(i);
				}
				break;


				}


			}
			break;

		case optypeindex(OPTYPE_FLOAT): {
			//
			// OPCODES with an double following in the wordCcode go in this switch
			// double d is the number extracted from the wordCode
			//
			double *d = (double *) op;
			op += bytesToFullWords(sizeof(double));
			switch(opcode) {

				case OP_PUSHFLOAT: {
					stack->pushDouble(*d);
				}
				break;

			}
		}
		break;

		case optypeindex(OPTYPE_LONG): {
    		qint64 *ll = (qint64 *) op;
    		op += bytesToFullWords(sizeof(qint64));     // advance by 2 words
    		switch(opcode) {
        		case OP_PUSHLONG:
            		stack->pushLong(*ll);
            		break;
    		}
		}
		break;

		case optypeindex(OPTYPE_STRING): {
			//
			// OPCODES with a string in the wordCcode go in this switch
			// int len will be set to the length and
			// QString s will be the string extracted from the wordCode
			//
			int len = strlen((char *) op) + 1;
			QString s = QString::fromUtf8((char *) op);
			op += bytesToFullWords(len);

			switch(opcode) {
				case OP_PUSHSTRING: {
					// push string from compiled bytecode
					stack->pushQString(s);
				}
				break;
			}
		}
		break;

		case optypeindex(OPTYPE_LABEL): {
			//
			// OPCODES with an integer wordCode label/symbol number go here
			// the symbol is looked up in the symtableaddress and changed to an array location
			// and stored in i BEFORE the OPCODES are executed
			//
			// int i is the new execution location offset within the array wordCode
			//
			int l = *op;	// label/symbol number
			int i = symtableaddress[l]; // address
			op++;

			// if address is -1 then this is not a function, subroutine or label
			if (i < 0) {
				//chose the proper error code if there is no valid address for this label/function/subroutine
				switch (opcode) {
				case OP_CALLFUNCTION:
					error->q(ERROR_NOSUCHFUNCTION, l);
					break;
				case OP_CALLSUBROUTINE:
					error->q(ERROR_NOSUCHSUBROUTINE, l);
					break;
				default:
					error->q(ERROR_NOSUCHLABEL, l);
					break;
				}
			}

			switch(opcode) {
				case OP_GOTO: {
					if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL && symtableaddresstype[l]!=ADDRESSTYPE_SYSTEMCALL){
						error->q(ERROR_NOSUCHLABEL, l);
					}else{
						op = wordCode + i;
					}
				}
				break;

				case OP_BRANCH: {
					// goto if true
					int val = stack->popBool();

					if (val == 0) { // jump on false
						op = wordCode + i;
					}
				}
				break;

				case OP_GOSUB: {
					if(symtableaddresstype[l]!=ADDRESSTYPE_LABEL){
						error->q(ERROR_NOSUCHLABEL, l);
					}else{
						// setup return
						callstack->push(op);
						// do jump
						op = wordCode + i;
					}
				}
				break;

				case OP_CALLFUNCTION: {
					//OP_CALLFUNCTION is used when program expect the result to be pushed on stack
					int a = stack->popInt(); // number of arguments pushed on stack
					if(symtableaddresstype[l]!=ADDRESSTYPE_FUNCTION){
						error->q(ERROR_NOSUCHFUNCTION, l);
					}else if(symtableaddressargs[l]!=a){
						//the number of arguments passed does not match FUNCTION definition
						error->q(ERROR_ARGUMENTCOUNT);
					}else{
						// setup return
						callstack->push(op);
						// do jump
						op = wordCode + i;
					}
				}
				break;


				case OP_CALLSUBROUTINE: {
				//OP_CALLSUBROUTINE is used to call subroutines
					int a = stack->popInt(); // number of arguments pushed on stack
					if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
						error->q(ERROR_NOSUCHSUBROUTINE, l);
					}else if(symtableaddressargs[l]!=a){
						//the number of arguments passed does not match SUBROUTINE definition
						error->q(ERROR_ARGUMENTCOUNT);
					}else{
						// setup return
						callstack->push(op);
						// do jump
						op = wordCode + i;
					}
				}
				break;

				case OP_ONERRORGOSUB: {
					if(symtableaddresstype[l]==ADDRESSTYPE_LABEL){
						// push onerror address
						onerrorstack->push(wordCode + i);
					}else{
						error->q(ERROR_NOSUCHLABEL, l);
					}
				}
				break;

				case OP_ONERRORCALL: {
					if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
						error->q(ERROR_NOSUCHSUBROUTINE, l);
					}else if(symtableaddressargs[l]!=0){
						error->q(ERROR_ONERRORSUB, l);
					}else{
						// push onerror address
						onerrorstack->push(wordCode + i);
					}
				}
				break;

				case OP_ONERRORCATCH: {
					// setup try/catch trap
					// label used is an internal generated label (is safe without checking it)
					trycatchframe *temp = new trycatchframe;
					temp->catchAddr = wordCode + i;
					temp->recurseLevel = variables->getrecurse();
					temp->stackSize = stack->height();
					temp->next = trycatchstack;
					trycatchstack = temp;
				}
				break;

				case OP_OFFERRORCATCH: {
					// no error in try/catch trap
					// delete the trap from the try/catch stack and jump over the CATCH part

					//  search if there is a corresponding trap in stack
					//  (search by catchAddr which must be the same with current *op)
					int recurse = variables->getrecurse();
					trycatchframe *temp_trycatchstack = trycatchstack;
					while (temp_trycatchstack!=NULL){
						if(temp_trycatchstack->catchAddr==op && recurse==temp_trycatchstack->recurseLevel){
							//  we found it - delete all nested traps inside it
							trycatchframe *temp;
							do{
								temp = trycatchstack;
								trycatchstack=trycatchstack->next;
								delete(temp);
							}while(temp_trycatchstack!=temp);
							break;
						}
						temp_trycatchstack = temp_trycatchstack->next;
					}
					// do jump to the specified address
					op = wordCode + i;
				}
				break;

				case OP_EXITFOR: {
					forframe *temp = forstack;
					if (!temp) {
						error->q(ERROR_NEXTNOFOR);
					} else {
						forstack = temp->next;
						if (temp->forFrameType==FORFRAMETYPE_FOREACH_ARRAY || temp->forFrameType==FORFRAMETYPE_FOREACH_MAP) {
							delete temp->foreach_de;
						}
						delete temp;
						op = wordCode + i;
					}

				}
				break;
				
				
				case OP_ONSTOPCALL: {
					if(symtableaddresstype[l]!=ADDRESSTYPE_SUBROUTINE){
						error->q(ERROR_NOSUCHSUBROUTINE, l);
					}else if(symtableaddressargs[l]!=0){
						error->q(ERROR_ONERRORSUB, l);
					}else{
						onstopaddr = (wordCode + i);
					}
				}
				break;

				case OP_PUSHLABEL: {
					// get a label fom the wordcode and push the offset address
					stack->pushInt(i);
				}
				break;


			}
		}
			break;
			 case optypeindex(OPTYPE_NONE): {
			switch(opcode) {
				case OP_NOP:
					break;

				case OP_END: {
					return -1;
				}
				break;


				case OP_RETURN: {
					int* addr = callstack->pop();
					if (addr) {
						op=addr;
					} else {
						error->q(ERROR_UNEXPECTEDRETURN);
					}
				}
				break;


			   case OP_OPENSERIAL: {
					int flow = stack->popInt();
					int parity = stack->popInt();
					int stop = stack->popInt();
					int data = stack->popInt();
					int baud = stack->popInt();
					QString name = stack->popQString();
					int fn = stack->popInt();
#if !defined(BASIC256_ENABLE_SERIAL)
					(void)flow;
					(void)parity;
					(void)stop;
					(void)data;
					(void)baud;
					(void)name;
					(void)fn;
					error->q(ERROR_NOTAVAILABLE);
# else
					if (fn<0||fn>=NUMFILES) {
						error->q(ERROR_FILENUMBER);
					} else {
						// close file number if open
						if (filehandle[fn] != NULL) {
							filehandle[fn]->close();
							filehandle[fn] = NULL;
						}
						// create file filehandle
						QSerialPort *p = new QSerialPort();
						if (p == NULL) {
							error->q(ERROR_FILEOPEN);
						} else {
							p->setPortName(name);
							p->setReadBufferSize(SERIALREADBUFFERSIZE);
							if (!error->pending()) {
								if (!p->open(QIODevice::ReadWrite)) {
									error->q(ERROR_FILEOPEN);
								} else {
									// successful open
									filehandle[fn] = p;
									filehandletype[fn] = 2;
									// set the parameters
									if (!p->setBaudRate(baud)) error->q(ERROR_SERIALPARAMETER);
									switch (data) {
										case 5:
											if(!p->setDataBits(QSerialPort::Data5)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 6:
											if(!p->setDataBits(QSerialPort::Data6)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 7:
											if(!p->setDataBits(QSerialPort::Data7)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 8:
											if(!p->setDataBits(QSerialPort::Data8)) error->q(ERROR_SERIALPARAMETER);
											break;
										default: error->q(ERROR_SERIALPARAMETER);
									}
									switch (stop) {
										case 1:
											if(!p->setStopBits(QSerialPort::OneStop)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setStopBits(QSerialPort::TwoStop)) error->q(ERROR_SERIALPARAMETER);
											break;
										default: error->q(ERROR_SERIALPARAMETER);
									}
									switch (parity) {
										case 0:
											if(!p->setParity(QSerialPort::NoParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 1:
											if(!p->setParity(QSerialPort::OddParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setParity(QSerialPort::EvenParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 3:
											if(!p->setParity(QSerialPort::SpaceParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 4:
											if(!p->setParity(QSerialPort::MarkParity)) error->q(ERROR_SERIALPARAMETER);
											break;
										default:
											error->q(ERROR_SERIALPARAMETER);
									}
									switch (flow) {
										case 0:
											if(!p->setFlowControl(QSerialPort::NoFlowControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 1:
											if(!p->setFlowControl(QSerialPort::HardwareControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										case 2:
											if(!p->setFlowControl(QSerialPort::SoftwareControl)) error->q(ERROR_SERIALPARAMETER);
											break;
										default:
											error->q(ERROR_SERIALPARAMETER);
									}
								}
							}
						}
					}
#endif
				}
				break;


				case OP_READ: {
    				int fn = stack->popInt();
    				if (fn < 0 || fn >= NUMFILES) {
        				error->q(ERROR_FILENUMBER);
        				stack->pushInt(0);
    				} else {
        				if (filehandle[fn] == nullptr) {
            				error->q(ERROR_FILENOTOPEN);
            				stack->pushInt(0);
        				} else {
            				char c = ' ';
            				bool readmore = true;
            				// Skip leading whitespace
            				do {
                				filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
                				readmore = filehandle[fn]->getChar(&c);
            				} while (readmore && (c == ' ' || c == '\t' || c == '\n'));
            				// Read token into QByteArray — grows automatically, no manual realloc
            				QByteArray token;
            				if (readmore) {
                				do {
                    				token.append(c);
                    				filehandle[fn]->waitForReadyRead(FILEREADTIMEOUT);
                    				readmore = filehandle[fn]->getChar(&c);
                				} while (readmore && c != ' ' && c != '\t' && c != '\n');
            				}
            				stack->pushQString(QString::fromUtf8(token));
            				goto nextop;
        				}
    				}
				}
				break;


				case OP_INT: {
					// bigger integer safe (trim floating point off of a float)
					double val = stack->popDouble();
					double intpart;
					val = modf(val + (val>0?EPSILON:-EPSILON), &intpart);
					if (intpart >= LONG_MIN && intpart <= LONG_MAX) {
						stack->pushLong(intpart);
					} else {
						stack->pushDouble(intpart);
					}
				}
				break;


				case OP_FLOAT: {
					double val = stack->popDouble();
					stack->pushDouble(val);
				}
				break;

				case OP_STRING: {
					stack->pushQString(stack->popQString());
				}
				break;

				case OP_NOISE: {
					// NOISE(x) walks a line through the two dimensional field,
					// NOISE(x,y) samples it directly and NOISE(x,y,z) samples the
					// three dimensional field - the grammar pushes 1, 2 or 3 to say
					// which form was written.
					int dims = stack->popInt();
					if (dims == 3) {
						double z = stack->popDouble();
						double y = stack->popDouble();
						double x = stack->popDouble();
						stack->pushDouble(OpenSimplex2::noise3(noiseSeed, x, y, z));
					} else if (dims == 2) {
						double y = stack->popDouble();
						double x = stack->popDouble();
						stack->pushDouble(OpenSimplex2::noise2(noiseSeed, x, y));
					} else {
						double x = stack->popDouble();
						stack->pushDouble(OpenSimplex2::noise1(noiseSeed, x));
					}
				}
				break;

				case OP_RAND: {
					double r = ((double) rand() * (double) RAND_MAX + (double) rand()) / double_random_max;
					stack->pushDouble(r);
				}
				break;

				case OP_SEED: {
					unsigned int seed = stack->popLong();
					srand(seed);
					// one SEED covers both generators, so a program that seeds gets
					// the same RAND sequence and the same NOISE field every run
					noiseSeed = (int64_t) seed;
				}
				break;

				case OP_PAUSE: {
					double val = stack->popDouble();
					if (val > 0) {
						sleeper->sleepSeconds(val);
					}
				}
				break;

				case OP_FRAMERATE: {
					// Cap the rate of the loop this statement sits in. Unlike PAUSE,
					// which sleeps for a fixed time ON TOP of the drawing and so
					// yields 1/(draw + delay), this waits UNTIL the next frame's
					// deadline and so absorbs however long the drawing took.
					double target = stack->popDouble();
					if (!(target > 0)) {
						// FRAMERATE 0 turns the cap off and forgets the deadline, so a
						// later FRAMERATE n starts timing from that point
						frameRateSet = false;
						break;
					}
					double secs = 1.0 / target;
					if (secs > 3600.0) secs = 3600.0;	// a frame longer than an hour is not a frame rate
					std::chrono::steady_clock::duration period =
						std::chrono::duration_cast<std::chrono::steady_clock::duration>(
							std::chrono::duration<double>(secs));
					std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
					if (!frameRateSet || now > frameDeadline + period) {
						// First frame, or a frame so slow we are already a whole frame
						// past due. Resync instead of carrying the deficit forward --
						// catching up would run a burst of zero-length frames and make
						// one slow frame look like a stutter that spreads.
						frameDeadline = now + period;
						frameRateSet = true;
					} else {
						// A deadline already past returns at once, which is how a small
						// overrun is clawed back over the following frames.
						sleeper->sleepUntil(frameDeadline);
						frameDeadline += period;
					}
				}
				break;

				case OP_LENGTH: {
					// array, map, or string
					DataElement *d = stack->popDE();			// RELEASE
					switch (DataElement::getType(d)) {
						case T_ARRAY:
						{
							// returns total number of elements - not dimensions
							stack->pushInt(d->arrayRows()*d->arrayCols());
						}
						break;
						case T_MAP:
						{
							stack->pushInt(d->mapLength());
						}
						break;
						default:
						{
							stack->pushInt(convert->getString(d).length());
						}
					}
					delete(d);
				}
				break;

				case OP_MID: {
				// unicode safe mid string
				//	   MID(string, pos, len)
				// pos - position. String indices begin at 1. If negative pos is given,
				//	   then position is starting from the end of the string,
				//	   where -1 is the last character position, -2 the second character from the end... and so on
				// len - number of characters to return. If negative number is given,
				//	   then the number of characters are removed and the result is returned

					int len = stack->popInt();
					int pos = stack->popInt();
					QString qtemp = stack->popQString();

					if (pos == 0){
						error->q(ERROR_STRSTART);
						stack->pushQString(QString(""));
					}else{
						if(pos<0)
							pos=qtemp.length()+pos;
						else
							pos--;

						if(len==0){
							stack->pushQString(QString(""));
						}else if(len>0){
							stack->pushQString(qtemp.mid(pos,len));
						}else{
							len=-len;
							//QString::remove is not acting like QString::mid when position is negative
							if(pos>=0)
								stack->pushQString(qtemp.remove(pos,len));
							else if(len+pos>=0) //pos is negative - there is something left to return
								stack->pushQString(qtemp.remove(0,len+pos));
							else
								stack->pushQString(qtemp);
						}
					}
				}
				break;


				case OP_MIDX: {
				// regex section string (MID regeX)
				//		 midx (expr, qtemp, start)
				// start - start position. String indices begin at 1. If negative start is given,
				//		 then position is starting from the end of the string,
				//		 where -1 is the last character position, -2 the second character from the end... and so on

					int start = stack->popInt();
					QRegularExpression expr(stack->popQString());
					if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
					QString qtemp = stack->popQString();

					if(start == 0) {
						error->q(ERROR_STRSTART);
						stack->pushQString(QString(""));
					} else {
						QRegularExpressionMatch m;
						if (start==1) {
							m = expr.match(qtemp);
						} else if (start>1){
							m = expr.match(qtemp.mid(start-1));
						}else{
							m = expr.match(qtemp.mid(qtemp.length()+start));
						}

						if (!m.hasMatch()) {
							// did not find it - return ""
							stack->pushQString(QString(""));
						} else {
							stack->pushQString(m.captured(0));
						}
					}
				}
				break;


				case OP_LEFT: {
				// unicode safe left string
				//	   LEFT(string, len)
				// len - number of characters to return. If negative number is given,
				//	   then the number of characters are removed and the result is returned
				// Eg.
				// LEFT("abcdefg", 3)  - returns "abc"  - translate: Return 3 characters starting from left
				// LEFT("abcdefg", -3) - returns "defg" - translate: Remove 3 characters starting from left

					int len = stack->popInt();
					QString qtemp = stack->popQString();

					if (len == 0) {
						stack->pushQString(QString(""));
					} else if (len > 0) {
						stack->pushQString(qtemp.left(len));
					} else {
						stack->pushQString(qtemp.remove(0,-len));
					}
				}
				break;


				case OP_RIGHT: {
				// unicode safe right string
				//	   RIGHT(string, len)
				// len - number of characters to return. If negative number is given,
				//	   then the number of characters are removed and the result is returned
				// Eg.
				// RIGHT("abcdefg", 3)  - returns "efg"  - translate: Return 3 characters starting from right
				// RIGHT("abcdefg", -3) - returns "abcd" - translate: Remove 3 characters starting from right

					int len = stack->popInt();
					QString qtemp = stack->popQString();

					if (len == 0) {
						stack->pushQString(QString(""));
					} else if (len > 0) {
						stack->pushQString(qtemp.right(len));
					} else {
						qtemp.chop(-len);
						stack->pushQString(qtemp);
					}
				}
				break;


				case OP_UPPER: {
					stack->pushQString(stack->popQString().toUpper());
				}
				break;

				case OP_LOWER: {
					stack->pushQString(stack->popQString().toLower());
				}
				break;

				case OP_ASC: {
					// unicode character sequence - return 16 bit number representing character
					QString qs = stack->popQString();
					stack->pushInt((int) qs[0].unicode());
				}
				break;


				case OP_CHR: {
					// convert a single unicode character sequence to string in utf8
					int code = stack->popInt();
					QChar temp[2];
					temp[0] = (QChar) code;
					temp[1] = (QChar) 0;
					QString qs = QString(temp,1);
					stack->pushQString(qs);
					qs = QString();
				}
				break;


				case OP_INSTR:
					{
						// unicode safe instr function
						//		 instr ( qhay , qstr , start , casesens)
						// start - start position. String indices begin at 1. If negative start is given,
						//		 then position is starting from the end of the string,
						//		 where -1 is the last character position, -2 the second character from the end... and so on
						// 0 sensitive (default) - opposite of QT
						Qt::CaseSensitivity casesens = (stack->popInt()==0?Qt::CaseSensitive:Qt::CaseInsensitive);
						int start = stack->popInt();
						QString qstr = stack->popQString();
						QString qhay = stack->popQString();

						int pos = 0;
						if(start == 0) {
							error->q(ERROR_STRSTART);
						} else if (start>0){
							pos = qhay.indexOf(qstr, start-1, casesens)+1;
						}else{
							int p = qhay.length()+start;
							if(p<0)
								p=0;
							pos = qhay.indexOf(qstr, p, casesens)+1;
						}
						stack->pushInt(pos);
					}
					break;


				case OP_INSTRX:
					{
						// regex instr
						//		 instrx (expr, qtemp, start)
						// start - start position. String indices begin at 1. If negative start is given,
						//		 then position is starting from the end of the string,
						//		 where -1 is the last character position, -2 the second character from the end... and so on
						int start = stack->popInt();
						QRegularExpression expr(stack->popQString());
						if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
						QString qtemp = stack->popQString();

						int pos=0;
						if(start == 0) {
							error->q(ERROR_STRSTART);
						} else if (start>0){
							pos = qtemp.indexOf(expr,start-1)+1;
						}else{
							int p = qtemp.length()+start;
							if(p<0)
								p=0;
							pos = qtemp.indexOf(expr, p)+1;
						}
						stack->pushInt(pos);
					}
					break;

				case OP_SIN:
					{
						double val = stack->popDouble();
						stack->pushDouble(sin(val));
					}
					break;

				case OP_COS:
					{
						double val = stack->popDouble();
						stack->pushDouble(cos(val));
					}
					break;

				case OP_TAN:
					{
						double val = stack->popDouble();
						val = tan(val);
						if (std::isinf(val)) {
							error->q(ERROR_INFINITY);
							stack->pushInt(0);
						} else {
							stack->pushDouble(val);
						}
					}
					break;

				case OP_ASIN:
					{
						double val = stack->popDouble();
						if (val<-1.0 || val>1.0) {
							error->q(ERROR_ASINACOSRANGE);
							stack->pushInt(0);
						} else {
							stack->pushDouble(asin(val));
						}
					}
					break;

				case OP_ACOS:
					{
						double val = stack->popDouble();
						if (val<-1.0 || val>1.0) {
							error->q(ERROR_ASINACOSRANGE);
							stack->pushInt(0);
						} else {
							stack->pushDouble(acos(val));
						}
					}
					break;

				case OP_ATAN:
					{
						double val = stack->popDouble();
						stack->pushDouble(atan(val));
					}
					break;

				case OP_CEIL:
					{
						double val = stack->popDouble();
						stack->pushInt(ceil(val));
					}
					break;

				case OP_FLOOR:
					{
						double val = stack->popDouble();
						stack->pushInt(floor(val));
					}
					break;

				case OP_DEGREES:
					{
						double val = stack->popDouble();
						stack->pushDouble(val * 180.0 / M_PI);
					}
					break;

				case OP_RADIANS:
					{
						double val = stack->popDouble();
						stack->pushDouble(val * M_PI / 180.0);
					}
					break;

				case OP_LOG:
					{
						double val = stack->popDouble();
						if (val<=0.0) {
							error->q(ERROR_LOGRANGE);
							stack->pushInt(0);
						} else {
							stack->pushDouble(log(val));
						}
					}
					break;

				case OP_LOGTEN:
					{
						double val = stack->popDouble();
						if (val<=0.0) {
							error->q(ERROR_LOGRANGE);
							stack->pushInt(0);
						} else {
							stack->pushDouble(log10(val));
						}
					}
					break;

				case OP_SQR:
					{
						double val = stack->popDouble();
						if (val<0.0) {
							error->q(ERROR_SQRRANGE);
							stack->pushInt(0);
						} else {
							stack->pushDouble(sqrt(val));
						}
					}
					break;

				case OP_EXP:
					{
						double val = stack->popDouble();
						val = exp(val);
						if (std::isinf(val)) {
							error->q(ERROR_INFINITY);
							stack->pushInt(0);
						} else {
							stack->pushDouble(val);
						}
					}
					break;

				case OP_CONCATENATE:
					// concatenate ";" operator - all types
					{
						QString sone = stack->popQString();
						QString stwo = stack->popQString();
						QString final = stwo + sone;
						if (final.length()>STRINGMAXLEN) {
							error->q(ERROR_STRINGMAXLEN);
							final.truncate(STRINGMAXLEN);
						}
						stack->pushQString(final);
					}
					break;

				case OP_ADD: {
						// in-place fast path - when both operands are already
						// numbers the answer is worked out in the element
						// underneath and the other one dropped, so an add
						// costs one pool release instead of two releases and
						// an allocation. Anything else falls through to the
						// general case below, which is unchanged.
						{
							DataElement *a1 = stack->peekDE(0);
							DataElement *a2 = stack->peekDE(1);
							if (a1 && a2) {
								if (a1->type==T_INT && a2->type==T_INT) {
									qint64 a = a2->intval + a1->intval;
									if(a>=INT_MIN && a<=INT_MAX) {
										a2->intval = a;
									} else {
										// overflow - promote to float, as below
										a2->floatval = (double)(a2->intval) + (double)(a1->intval);
										a2->type = T_FLOAT;
									}
									stack->dropTop();
									break;
								}
								if ((a1->type==T_INT || a1->type==T_FLOAT) &&
									(a2->type==T_INT || a2->type==T_FLOAT)) {
									double ans = (a2->type==T_INT ? (double)(a2->intval) : a2->floatval)
											   + (a1->type==T_INT ? (double)(a1->intval) : a1->floatval);
									if (std::isinf(ans)) {
										error->q(ERROR_INFINITY);
										ans = 0.0;
									}
									a2->floatval = ans;
									a2->type = T_FLOAT;
									stack->dropTop();
									break;
								}
							}
						}
						// integer and float safe ADD operation
						DataElement *one = stack->popDE();			// RELEASE
						DataElement *two = stack->popDE();			// RELEASE
						// add - if both integer then add as integers (if no ovverflow)
						// else if both are numbers then convert and add as floats
						// otherwise concatenate (string & number or strings)
						if (DataElement::getType(one)==T_INT && DataElement::getType(two)==T_INT) {
							qint64 a = two->intval + one->intval;
							if(a>=INT_MIN && a<=INT_MAX) {
								// integer add - fits in the 32-bit int range BASIC-256 promotes to
								// float beyond (matches tohex()/toradix()/the literal parser's own
								// 32-bit-wide semantics, not intval's full 64-bit storage capacity)
								stack->pushLong(a);
							}else{
								//overflow
								stack->pushDouble((double)(two->intval) + (double)(one->intval));
							}
						} else if(DataElement::getType(one)==T_INT && DataElement::getType(two)==T_FLOAT){
							double ans = two->floatval + (double) one->intval;
							if (std::isinf(ans)) {
								error->q(ERROR_INFINITY);
								ans = 0.0;
							}
							stack->pushDouble(ans);
						} else if(DataElement::getType(one)==T_FLOAT && DataElement::getType(two)==T_INT){
							double ans = (double) two->intval + one->floatval;
							if (std::isinf(ans)) {
								error->q(ERROR_INFINITY);
								ans = 0.0;
							}
							stack->pushDouble(ans);
						} else if(DataElement::getType(one)==T_FLOAT && DataElement::getType(two)==T_FLOAT){
							double ans = two->floatval + one->floatval;
							if (std::isinf(ans)) {
								error->q(ERROR_INFINITY);
								ans = 0.0;
							}
							stack->pushDouble(ans);
						} else {
							// concatenate (if one or both ar not numbers)
							QString sone = convert->getString(one);
							QString stwo = convert->getString(two);
							QString final = stwo + sone;
							if (final.length()>STRINGMAXLEN) {
								error->q(ERROR_STRINGMAXLEN);
								final.truncate(STRINGMAXLEN);
							}
							stack->pushQString(final);
						}
						delete one;
						delete two;
					}
					break;

				case OP_SUB: {
						// in-place fast path - see OP_ADD
						{
							DataElement *a1 = stack->peekDE(0);
							DataElement *a2 = stack->peekDE(1);
							if (a1 && a2) {
								if (a1->type==T_INT && a2->type==T_INT) {
									qint64 a = a2->intval - a1->intval;
									if(a>=INT_MIN && a<=INT_MAX) {
										a2->intval = a;
										stack->dropTop();
										break;
									}
									// overflow falls through to the float case
								}
								if ((a1->type==T_INT || a1->type==T_FLOAT) &&
									(a2->type==T_INT || a2->type==T_FLOAT)) {
									double ans = (a2->type==T_INT ? (double)(a2->intval) : a2->floatval)
											   - (a1->type==T_INT ? (double)(a1->intval) : a1->floatval);
									if (std::isinf(ans)) {
										error->q(ERROR_INFINITY);
										ans = 0.0;
									}
									a2->floatval = ans;
									a2->type = T_FLOAT;
									stack->dropTop();
									break;
								}
							}
						}
						// integer and float safe SUB operation
						DataElement *one = stack->popDE();			// RELEASE
						DataElement *two = stack->popDE();			// RELEASE
						if (DataElement::getType(one)==T_INT &&DataElement::getType(two)==T_INT) {
							qint64 a = two->intval - one->intval;
							if(a>=INT_MIN && a<=INT_MAX) {
								// integer subtract - fits in the 32-bit int range (see OP_ADD)
								stack->pushLong(a);
								delete one;
								delete two;
								break;
							}
						}
						// if we have a float or an overflow then fall through to float subtract
						double fone = convert->getFloat(one);
						double ftwo = convert->getFloat(two);
						double ans = ftwo - fone;
						if (std::isinf(ans)) {
							error->q(ERROR_INFINITY);
							stack->pushDouble(0.0);
						} else {
							stack->pushDouble(ans);
						}
						delete one;
						delete two;
					}
					break;

				case OP_MUL: {
						// in-place fast path - see OP_ADD. The string repeat
						// case (int * string) is not handled here and falls
						// through to the general case below.
						{
							DataElement *a1 = stack->peekDE(0);
							DataElement *a2 = stack->peekDE(1);
							if (a1 && a2) {
								if (a1->type==T_INT && a2->type==T_INT) {
									if(a2->intval==0 || a1->intval==0) {
										a2->intval = 0;
										stack->dropTop();
										break;
									}
									if (llabs(a1->intval) <= INT64_MAX / llabs(a2->intval)) {
										qint64 a = a2->intval * a1->intval;
										if(a>=INT_MIN && a<=INT_MAX) {
											a2->intval = a;
											stack->dropTop();
											break;
										}
									}
									// overflow - fall into the float case below
								}
								if ((a1->type==T_INT || a1->type==T_FLOAT) &&
									(a2->type==T_INT || a2->type==T_FLOAT)) {
									double ans = (a2->type==T_INT ? (double)(a2->intval) : a2->floatval)
											   * (a1->type==T_INT ? (double)(a1->intval) : a1->floatval);
									if (std::isinf(ans)) {
										error->q(ERROR_INFINITY);
										ans = 0.0;
									}
									a2->floatval = ans;
									a2->type = T_FLOAT;
									stack->dropTop();
									break;
								}
							}
						}
						// integer and float safe MUL operation
						DataElement *one = stack->popDE();			// RELEASE
						DataElement *two = stack->popDE();			// RELEASE
						if (DataElement::getType(one)==T_INT && DataElement::getType(two)==T_INT) {
							if(two->intval==0||one->intval==0) {
								stack->pushLong(0);
								delete one;
								delete two;
								break;
							} else {
								if (llabs(one->intval) <= INT64_MAX / llabs(two->intval)) {
									qint64 a = two->intval * one->intval;
									if(a>=INT_MIN && a<=INT_MAX) {
										// integer multiply - fits in the 32-bit int range (see OP_ADD);
										// the llabs/INT64_MAX check above only guards against genuine
										// 64-bit multiplication overflow (undefined behavior), it's not
										// the promote-to-float boundary itself
										stack->pushLong(a);
										delete one;
										delete two;
										break;
									}
									// else fall through to float multiply (32-bit overflow)
								}
								// else fall through to float multiply (would overflow 64-bit qint64 multiplication)
							}
						} else if (DataElement::getType(one)==T_INT && DataElement::getType(two)==T_STRING) {
							// string repeat like python
							stack->pushQString(two->stringval.repeated(one->intval));
							delete one;
							delete two;
							break;
						}
						// if we have a float or we overflow - fall through to float multiply
						double fone = convert->getFloat(one);
						double ftwo = convert->getFloat(two);
						double ans = ftwo * fone;
						if (std::isinf(ans)) {
							error->q(ERROR_INFINITY);
							stack->pushDouble(0.0);
						} else {
							stack->pushDouble(ans);
						}
						delete one;
						delete two;
					}
					break;

				case OP_ABS:
				{
					DataElement *one = stack->popDE();			// RELEASE
					if (DataElement::getType(one)==T_INT) {
						stack->pushLong(llabs(one->intval));
					} else {
						stack->pushDouble(fabs(convert->getFloat(one)));
					}
					delete one;
				}
				break;

				case OP_EX: {
					// always return a float value with power "^"
					double oneval = stack->popDouble();
					double twoval = stack->popDouble();
					double ans = pow(twoval, oneval);
					if (std::isinf(ans)) {
						error->q(ERROR_INFINITY);
						stack->pushDouble(0.0);
					} else {
						stack->pushDouble(ans);
					}
				}
				break;

				case OP_DIV: {
					// always return a float value with division "/"
					double oneval = stack->popDouble();
					double twoval = stack->popDouble();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushDouble(0.0);
					} else {
						double ans = twoval / oneval;
						if (std::isinf(ans)) {
							error->q(ERROR_INFINITY);
							stack->pushDouble(0.0);
						} else {
							stack->pushDouble(ans);
						}
					}
				}
				break;

				case OP_INTDIV: {
					qint64 oneval = stack->popLong();
					qint64 twoval = stack->popLong();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushLong(0);
					} else if (oneval==-1 && twoval==LLONG_MIN) {
						// LLONG_MIN \ -1 overflows: the result 2^63 is not
						// representable in a signed 64-bit int. Computing it is
						// undefined behavior (traps as SIGFPE on x86), so report
						// the range error instead.
						error->q(ERROR_LONGRANGE);
						stack->pushLong(0);
					} else {
						stack->pushLong(twoval / oneval);
					}
				}
				break;

				case OP_MOD: {
					qint64 oneval = stack->popLong();
					qint64 twoval = stack->popLong();
					if (oneval==0) {
						error->q(ERROR_DIVZERO);
						stack->pushLong(0);
					} else if (oneval==-1) {
						// any integer mod -1 is 0. Special-cased because
						// LLONG_MIN % -1 is undefined behavior (the quotient
						// 2^63 is not representable) and traps as SIGFPE on x86.
						stack->pushLong(0);
					} else {
						stack->pushLong(twoval % oneval);
					}
				}
				break;

				case OP_AND: {
					int one = stack->popBool();
					int two = stack->popBool();
					stack->pushBool(one && two);
				}
				break;

				case OP_OR: {
					int one = stack->popBool();
					int two = stack->popBool();
					stack->pushBool(one || two);
				}
				break;

				case OP_XOR: {
					int one = stack->popBool();
					int two = stack->popBool();
					stack->pushBool(!(one && two) && (one || two));
				}
				break;

				case OP_NOT: {
					int temp = stack->popBool();
					stack->pushBool(!temp);
				}
				break;

				case OP_NEGATE: {
					// integer safe negate
					DataElement *e = stack->popDE();			// RELEASE
					if (DataElement::getType(e)==T_INT) {
						if(e->intval<=LONG_MIN){
							stack->pushDouble( (double)e->intval * -1.0);
						}else{
							stack->pushLong(e->intval * -1);
						}
					} else {
						stack->pushDouble(convert->getFloat(e) * -1.0);
					}
					delete e;
				}
				break;

				case OP_EQUAL:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans==0);
				}
				break;

				case OP_NEQUAL:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans!=0);
				}
				break;

				case OP_GT:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans==1);
				}
				break;

				case OP_LTE:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans!=1);
				}
				break;

				case OP_LT:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans==-1);
				}
				break;

				case OP_GTE:{
					// borrowed: both are read by compare() before anything is pushed
					DataElement *two = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *one = stack->popDEborrow();		// DO NOT RELEASE
					int ans = convert->compare(one,two);
					stack->pushBool(ans!=-1);
				}
				break;

				case OP_IN:{
					DataElement *map = stack->popDE();			// RELEASE
					QString key = stack->popQString();
					stack->pushBool(map->mapKey(key));
					delete map;
				}
				break;

				// TEXTCOL and TEXTROW report where the next PRINT will land, so
				// LOCATE TEXTCOL, TEXTROW is a statement that does nothing.
				case OP_YEAR:
				case OP_MONTH:
				case OP_DAY:
				case OP_HOUR:
				case OP_MINUTE:
				case OP_SECOND: {
					time_t rawtime;
					struct tm * timeinfo;

					time ( &rawtime );
					timeinfo = localtime ( &rawtime );

					switch (opcode) {
						case OP_YEAR:
							stack->pushInt(timeinfo->tm_year + 1900);
							break;
						case OP_MONTH:
							stack->pushInt(timeinfo->tm_mon);
							break;
						case OP_DAY:
							stack->pushInt(timeinfo->tm_mday);
							break;
						case OP_HOUR:
							stack->pushInt(timeinfo->tm_hour);
							break;
						case OP_MINUTE:
							stack->pushInt(timeinfo->tm_min);
							break;
						case OP_SECOND:
							stack->pushInt(timeinfo->tm_sec);
							break;
					}
				}
				break;

				case OP_INCREASERECURSE:
				{
					// increase recursion level in variable hash
					//push forstack into forstacklevel
					const int level = variables->getrecurse();
					if(forstacklevelsize <= level){
						forstacklevel.resize(level+1);
						forstacklevelsize++;
					}
					forstacklevel[level] = forstack;
					forstack = NULL;
					variables->increaserecurse();
				}
				break;

				case OP_DECREASERECURSE:
				{
					// decrease recursion level in variable hash
					// and pop any unfinished for statements off of forstack
					decreaserecurse();
				}
				break;

				case OP_LASTERROR: {
					stack->pushInt(error->e);
				}
				break;

				case OP_LASTERRORLINE: {
					stack->pushInt(error->line);
				}
				break;

				case OP_LASTERROREXTRA: {
					stack->pushQString(error->extra);
				}
				break;

				case OP_LASTERRORMESSAGE: {
					stack->pushQString(error->getErrorMessage(symtable));
				}
				break;

				case OP_OFFERROR: {
					// pop a trap off of the on-error stack
					onerrorstack->drop();
				}
				break;


				case OP_MD5: {
					QString stuff = stack->popQString();
					char *digest = MD5(stuff.toUtf8().data()).hexdigest();
					stack->pushQString(digest);
					free(digest);
				}
				break;

				case OP_BITSHIFTL: {
				// safe bit left shift
				// The standard says: "If the value of the right operand is negative or is greater than
				// or equal to the width of the promoted left operand, the behavior is undefined."
					int n = stack->popInt();
					quint64 a = (quint64)stack->popLong();
					if(n>=0){
						if(n>=((int)sizeof(a))*8){
							stack->pushInt(0);
						}else{
							a = a << n;
							stack->pushLong(a);
						}
					}else{
						// if right operand is negative, shift in the opposite direction
						if(n<=-((int)sizeof(a))*8){
							stack->pushInt(0);
						}else{
							a = a >> (n*-1);
							stack->pushLong(a);
						}
					}
				}
				break;

				case OP_BITSHIFTR: {
				// safe bit right shift
				// The standard says: "If the value of the right operand is negative or is greater than
				// or equal to the width of the promoted left operand, the behavior is undefined."
					int n = stack->popInt();
					quint64 a = (quint64)stack->popLong();
					if(n>=0){
						if(n>=((int)sizeof(a))*8){
							stack->pushInt(0);
						}else{
							a = a >> n;
							stack->pushLong(a);
						}
					}else{
						// if right operand is negative, shift in the opposite direction
						if(n<=-((int)sizeof(a))*8){
							stack->pushInt(0);
						}else{
							a = a << (n*-1);
							stack->pushLong(a);
						}
					}
				}
				break;

				case OP_BINARYOR: {
					qint64 a = stack->popLong();
					qint64 b = stack->popLong();
					a = a | b;
					stack->pushLong(a);
				}
				break;

				case OP_BINARYAND: {
					DataElement *one = stack->popDE();			// RELEASE
					DataElement *two = stack->popDE();			// RELEASE
					// if both are numbers then convert to long and bitwise and
					// otherwise concatenate (string & number or strings)
					if ((DataElement::getType(one)==T_INT || DataElement::getType(one)==T_FLOAT) && (DataElement::getType(two)==T_INT || DataElement::getType(two)==T_FLOAT)) {
						qint64 a = convert->getLong(one);
						qint64 b = convert->getLong(two);
						a = a&b;
						stack->pushLong(a);
					} else {
						// concatenate (if one or both at not numbers or cant be converted to numbers)
						QString sone = convert->getString(one);
						QString stwo = convert->getString(two);
						QString final = stwo + sone;
						if (final.length()>STRINGMAXLEN) {
							final.truncate(STRINGMAXLEN);
							error->q(ERROR_STRINGMAXLEN);
						}
						stack->pushQString(stwo + sone);
					}
					delete one;
					delete two;
				}
				break;

				case OP_BINARYNOT: {
					qint64 a = stack->popLong();
					a = ~a;
					stack->pushLong(a);
				}
				break;

				case OP_REPLACE: {
					// unicode safe replace function

				   // 0 sensitive (default) - opposite of QT
					Qt::CaseSensitivity casesens = (stack->popInt()==0?Qt::CaseSensitive:Qt::CaseInsensitive);

					QString qto = stack->popQString();
					QString qfrom = stack->popQString();
					QString qhaystack = stack->popQString();

					stack->pushQString(qhaystack.replace(qfrom, qto, casesens));
				}
				break;

				case OP_REPLACEX: {
					// regex replace function

					QString qto = stack->popQString();
					QRegularExpression expr(stack->popQString());
					if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
					QString qhaystack = stack->popQString();

					stack->pushQString(qhaystack.replace(expr, qto));
				}
				break;

				case OP_COUNT: {
					// unicode safe count function

					// 0 sensitive (default) - opposite of QT
					Qt::CaseSensitivity casesens = (stack->popInt()==0?Qt::CaseSensitive:Qt::CaseInsensitive);

					QString qneedle = stack->popQString();
					QString qhaystack = stack->popQString();

					stack->pushInt((int) (qhaystack.count(qneedle, casesens)));
				}
				break;

				case OP_COUNTX: {
					// regex count function

					QRegularExpression expr(stack->popQString());
					if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
					QString qhaystack = stack->popQString();

					stack->pushInt((int) (qhaystack.count(expr)));
				}
				break;

				case OP_MSEC: {
					// Return number of milliseconds the BASIC256 program has been running
					stack->pushInt((int) (runtimer.elapsed()));
				}
				break;

				case OP_REGEXMINIMAL: {
					// set the regular expression minimal flag (true = not greedy)
					regexMinimal = stack->popInt()!=0;
				}
				break;

				case OP_FROMRADIX: {
					bool ok;
					int base = stack->popInt();
					QString n = stack->popQString();
					if (base>=2 && base <=36) {
						quint64 dec = n.toULongLong(&ok, base);
						if (ok) {
							// values that fit in 32 bits reinterpret as a signed
							// 32-bit int (matches tohex()/tobinary()/tooctal()'s
							// OP_TORADIX and the 0x/0b/0o literal parser in
							// basicParse.y); only genuinely larger values are
							// kept as a real 64-bit long.
							if (dec <= 0xFFFFFFFFULL)
								stack->pushLong((int)(quint32)dec);
							else
								stack->pushLong((qint64)dec);
						} else {
							error->q(ERROR_RADIXSTRING);
							stack->pushLong(0);
						}
					} else {
						error->q(ERROR_RADIX);
						stack->pushLong(0);
					}
				}
				break;

				case OP_TORADIX: {
					int base = stack->popInt();
					quint32 n = (quint32)stack->popLong();
					if (base>=2 && base <=36) {
						QString out;
						out.setNum(n, base);
						stack->pushQString(out);
					} else {
						error->q(ERROR_RADIX);
						stack->pushQString(QString("0"));
					}
				}
				break;


				case OP_DEBUGINFO: {
					// get info about BASIC256 runtime and return as a string
					// put totally undocumented stuff HERE
					// NOT FOR HUMANS TO USE
					int what = stack->popInt();
					switch (what) {
						case 1:
							// stack height and content
							mymutex->lock();
							emit(outputReady(stack->debug()));
							waitCond->wait(mymutex);
							mymutex->unlock();
							stack->pushInt(stack->height());
							break;
						case 2:
							// type of top stack element
							stack->pushInt(stack->peekType());
							break;
						case 3:
							// number of symbols - display them to output area
							{
								for(int i=0; i<numsyms; i++) {
									mymutex->lock();
									emit(outputReady(QString("SYM %1 %2 LOC %3\n").arg(i).arg(symtable[i],-32).arg(symtableaddress[i],8,16,QChar('0'))));
									waitCond->wait(mymutex);
									mymutex->unlock();
								}
								stack->pushInt(numsyms);
							}
							break;
						case 4:
							// dump the program object code
							{
								int *o = wordCode;
								while (o <= wordCode + wordOffset) {
									mymutex->lock();
									unsigned int offset = o-wordCode;
									int currentop = *o;
									o++;
									if (optype(currentop) == OPTYPE_NONE)	{
										emit(outputReady(QString("%1 %2\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20)));
									} else if (optype(currentop) == OPTYPE_INT) {
										//op has one Int arg
										emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o)));
										o++;
									} else if (optype(currentop) == OPTYPE_VARIABLE) {
										//op has one Int arg
										emit(outputReady(QString("%1 %2 (%3) %4\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o).arg(symtable[(int) *o])));
										o++;
									} else if (optype(currentop) == OPTYPE_VAR_VAR) {
										//op has one wto int (var)
										emit(outputReady(QString("%1 %2 (%3) %4 (%5) %6\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((int) *o).arg(symtable[(int) *o]).arg((int) *(o+1)).arg(*(o+1)>0?symtable[(int) *(o+1)]:"__none__")));
										o+=2;
									} else if (optype(currentop) == OPTYPE_LABEL) {
										//op has one Int arg (label - lookup the address from the symtableaddress
										emit(outputReady(QString("%1 %2 (%3) %4\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg(symtableaddress[(int) *o],8,16,QChar('0')).arg(symtable[(int) *o])));
										o++;
									} else if (optype(currentop) == OPTYPE_FLOAT) {
										// op has a single double arg
										emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg( *(double*)o, 0, 'g', 10)));
										o += bytesToFullWords(sizeof(double));
									} else if (optype(currentop) == OPTYPE_STRING) {
										// op has a single null terminated String arg
										emit(outputReady(QString("%1 %2 \"%3\"\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg((char*) o)));
										int len = bytesToFullWords(strlen((char*) o) + 1);
										o += len;
									} else if (optype(currentop) == OPTYPE_LONG) {
    									emit(outputReady(QString("%1 %2 %3\n").arg(offset,8,16,QChar('0')).arg(opname(currentop),-20).arg(*(qint64*)o)));
    									o += bytesToFullWords(sizeof(qint64));          // o += 2
									}
									waitCond->wait(mymutex);
									mymutex->unlock();
								}
							}
							stack->pushInt(0);
							break;
						case 5:
							// dump the variables
							{
								mymutex->lock();
								emit(outputReady(variables->debug()));
								waitCond->wait(mymutex);
								mymutex->unlock();
							}
							stack->pushInt(0);
							break;
						case 6:
							// size of int
							stack->pushInt(sizeof(int));
							break;
						case 7:
							// size of long
							stack->pushInt(sizeof(long));
							break;
						case 8:
							// size of long long
							stack->pushInt(sizeof(long long));
							break;

						default:
							stack->pushQString("");
					}
				}
				break;

				case OP_STACKSWAP: {
					// swap the top of the stack
					// 0, 1, 2, 3...  becomes 1, 0, 2, 3...
					stack->swap();
				}
				break;

				case OP_STACKSWAP2: {
					// swap the top two pairs of the stack
					// 0, 1, 2, 3...  becomes 2,3, 0,1...
					stack->swap2();
				}
				break;

				case OP_STACKDUP: {
					// duplicate top stack entry
					stack->dup();
				}
				break;

				case OP_STACKDUP2: {
					// duplicate top 2 stack entries
					stack->dup2();
				}
				break;

				case OP_STACKTOPTO2: {
					// move the top of the stack under the next two
					// 0, 1, 2, 3...  becomes 1, 2, 0, 3...
					stack->topto2();
				}
				break;

				case OP_THROWERROR: {
					// Throw a user defined error number
					int fn = stack->popInt();
					error->q(fn);
				}
				break;

				 case OP_TYPEOF: {
					// return type of expression (top of the stack)
					DataElement *e = stack->popDE();			// RELEASE
					stack->pushInt(DataElement::getType(e));
					delete e;
				}
				break;

				case OP_ISNUMERIC: {
					// return if data element is numeric
					DataElement *e = stack->popDE();			// RELEASE
					stack->pushInt(convert->isNumeric(e));
					delete e;
				}
				break;

				case OP_LTRIM: {
					QString s = stack->popQString();
					int l = s.length();
					int p = 0;
					while(p<l && s.at(p).isSpace()) {
						p++;
					}
					stack->pushQString(s.mid(p));
				}
				break;

				case OP_RTRIM: {
					QString s = stack->popQString();
					int l = s.length();
					int p = l;
					while(p>0 && s.at(p-1).isSpace()) {
						p--;
					}
					stack->pushQString(s.mid(0,p));
				}
				break;

				case OP_TRIM: {
					QString s = stack->popQString();
					stack->pushQString(s.trimmed());
				}
				break;

				case OP_IMPLODE: {
					QString coldelim = stack->popQString();
					QString rowdelim = stack->popQString();
					QString stuff = "";
					DataElement *d = stack->popDE();			// RELEASE
					int rows = d->arrayRows();
					int cols = d->arrayCols();
					for (int row=0; row<rows; row++) {
						if (row!=0) stuff.append(rowdelim);
						for (int col=0; col<cols; col++) {
								if (col!=0) stuff.append(coldelim);
								stuff.append(convert->getString(d->arrayGetData(row,col)));			// DONT RELEASE
						}
					}
					stack->pushQString(stuff);
					delete d;
				}
				break;

				case OP_SERIALIZE: {
					// rows,columns,typedata
					DataElement *e = stack->popDE();			// RELEASE
					DataElement *temp;
					switch (DataElement::getType(e)) {
						case T_ARRAY:
						{
							QString stuff = "";
							int rows = e->arrayRows();
							int cols = e->arrayCols();
							for (int row=0; row<rows; row++) {
								if (row!=0) stuff.append(SERALIZE_DELIMITER);
								for (int col=0; col<cols; col++) {
										if (col!=0) stuff.append(SERALIZE_DELIMITER);
										temp = e->arrayGetData(row,col);			// DONT RELEASE
										switch (DataElement::getType(temp)) {
											case T_STRING:
												stuff.append(SERALIZE_STRING + QString::fromUtf8(temp->stringval.toUtf8().toHex()) );
												break;
											case T_FLOAT:
												stuff.append(SERALIZE_FLOAT + QString::number(temp->floatval));
												break;
											case T_INT:
												stuff.append(SERALIZE_INT + QString::number(temp->intval));
												break;
											default:
												stuff.append(SERALIZE_UNASSIGNED);
												break;
										}
								}
							}
							stack->pushQString(QString::number(rows) + SERALIZE_DELIMITER + QString::number(cols) + SERALIZE_DELIMITER + stuff);
						}
						break;
						case T_MAP:
						{
							QString stuff = "M";
							stuff.append(SERALIZE_DELIMITER);
							stuff.append(QString::number(e->mapLength()));
							{
								std::map<std::string, DataElement*>::iterator it;
								for (it = e->map->data.begin(); it != e->map->data.end(); it++ )
								{
									stuff.append(SERALIZE_DELIMITER);
									stuff.append(SERALIZE_STRING + QString::fromStdString(it->first).toUtf8().toHex() );
									stuff.append(SERALIZE_DELIMITER);
									switch (DataElement::getType(it->second)) {
										case T_STRING:
											stuff.append(SERALIZE_STRING + QString::fromUtf8(it->second->stringval.toUtf8().toHex()) );
											break;
										case T_FLOAT:
											stuff.append(SERALIZE_FLOAT + QString::number(it->second->floatval));
											break;
										case T_INT:
											stuff.append(SERALIZE_INT + QString::number(it->second->intval));
											break;
										default:
											stuff.append(SERALIZE_UNASSIGNED);
											break;
									};
								}
							}
						stack->pushQString(stuff);
						}
						break;
						default:
						{
							error->q(ERROR_ARRAYORMAPEXPR);
						}
					}
					delete e;
				}
				break;


				case OP_EXPLODE:
				case OP_EXPLODEX: {
					// unicode safe explode a string to an array
					// pushed on the stack for use in assign and other places
					DataElement *d = new DataElement();			// RELEASE
					
					Qt::CaseSensitivity casesens = Qt::CaseSensitive;

					if (opcode!=OP_EXPLODEX) {
						// 0 sensitive (default) - opposite of QT
						casesens = (stack->popInt()==0?Qt::CaseSensitive:Qt::CaseInsensitive);
					}
					QString qneedle = stack->popQString();
					QString qhaystack = stack->popQString();

					QStringList list;
					if(opcode==OP_EXPLODE) {
						//list = qhaystack.split(qneedle, QString::KeepEmptyParts , casesens);
						list = qhaystack.split(qneedle, Qt::KeepEmptyParts , casesens);
					} else {
						QRegularExpression expr(qneedle);
						if (regexMinimal) expr.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
						if (expr.captureCount()>0) {
							// if we have captures in our regex then return them
							QRegularExpressionMatch m = expr.match(qhaystack);
							list = m.capturedTexts();
						} else {
							// if it is a simple regex without captures then split
							list = qhaystack.split(expr, Qt::KeepEmptyParts);
						}
					}
					
					// put list elements into array
					d->arrayDim(1, list.size(), false);
					for(int y=0; y<list.size(); y++) {
						// fill the string array
						DataElement *temp = new DataElement(list.at(y));
						d->arraySetData(0,y,temp);
						delete temp;
					}
					stack->pushDE(d);
					delete d;
				}
				break;

				case OP_UNSERIALIZE: {
					bool good;

					QString data = stack->popQString();
					QStringList list = data.split(SERALIZE_DELIMITER);
					
					if (list.count()>=3) {
						if(list[0].at(0).toLatin1()=='M') {
							DataElement *d = new DataElement();			// RELEASE
							d->mapDim();
							int pairs = list[1].toInt(&good);
							for (int i=0; i<pairs; i++) {
								DataElement *dat;			// RELEASE
								QString key = QString::fromUtf8(QByteArray::fromHex(list[i*2+2].mid(1).toUtf8()).data());
								switch (list[i*2+3].at(0).toLatin1()) {
									case SERALIZE_STRING:
										dat = new DataElement(QString::fromUtf8(QByteArray::fromHex(list[i*2+3].mid(1).toUtf8()).data()));
										break;
									case SERALIZE_FLOAT:
										dat = new DataElement(list[i*2+3].mid(1).toDouble());
										break;
									case SERALIZE_INT:
										dat = new DataElement(list[i*2+3].mid(1).toLongLong());
										break;
									case SERALIZE_UNASSIGNED:
										dat = new DataElement();
										break;
									default:
										dat = new DataElement();
										error->q(ERROR_UNSERIALIZEFORMAT);
								}
								d->mapSetData(key,dat);
								delete dat;
							}
							stack->pushDE(d);
							delete d;
						} else {
							// array
							DataElement *d = new DataElement();			// RELEASE
							int rows = list[0].toInt(&good);
							int cols = list[1].toInt(&good);
							d->arrayDim(rows, cols, false);
							if (list.count()==rows*cols+2) {
								for (int row=0; row<rows; row++) {
									for (int col=0; col<cols; col++) {
										DataElement *dat;			// RELEASE
										int i = row * cols + col + 2;
										switch (list[i].at(0).toLatin1()) {
											case SERALIZE_STRING:
												dat = new DataElement(QString::fromUtf8(QByteArray::fromHex(list[i].mid(1).toUtf8()).data()));
												break;
											case SERALIZE_FLOAT:
												dat = new DataElement(list[i].mid(1).toDouble());
												break;
											case SERALIZE_INT:
												dat = new DataElement(list[i].mid(1).toLongLong());
												break;
											case SERALIZE_UNASSIGNED:
												dat = new DataElement();
												break;
											default:
												dat = new DataElement();
												error->q(ERROR_UNSERIALIZEFORMAT);
										}
										d->arraySetData(row,col,dat);
										delete dat;
									}
								}
							}
							stack->pushDE(d);
							delete d;
						}
					} else {
						error->q(ERROR_UNSERIALIZEFORMAT);
					}
				}
				break;

				case OP_LIST2ARRAY: {
					// pop a list of values off of stack and push
					// it back on as a single DataElement with the data as array
					// remember this is not associated with variable
					// will make a full square array but will leave unassigned elements where they were
					// not included in the list
					const int ydim = stack->popInt();
					const int xdim = stack->popInt();
					
					DataElement *edest = new DataElement();			// RELEASE
					edest->arrayDim(xdim, ydim, false);

					for(int row = xdim-1; row>=0; row--) {
						int thisy=stack->popInt();
						for(int col= thisy-1; col >= 0; col--) {
							DataElement *e = stack->popDE();			// RELEASE
							edest->arraySetData(row,col, e);
							delete e;
						}
					}
					stack->pushDE(edest);
					delete edest;
				}
				break;


				case OP_DOT: {
					// the dot product of two vectors.  Any two arrays holding
					// the same number of elements will do - a row, a column or
					// the output of MAT TRN - because what is multiplied is
					// element by element in the order they are stored.
					DataElement *b = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *a = stack->popDEborrow();		// DO NOT RELEASE
					if (!vecOperand(a) || !vecOperand(b)) {
						stack->pushLong(0);
						break;
					}
					const int n = vecSize(a);
					if (vecSize(b)!=n) {
						error->q(ERROR_VECDIM);
						stack->pushLong(0);
						break;
					}
					// whole numbers in give a whole number out, the way the
					// MAT statements work, until one will not fit
					MatNum sum = matInt(0);
					for (int k=0; k<n; k++) {
						sum = matNumAdd(sum, matNumMul(vecGet(a, k, convert), vecGet(b, k, convert)));
					}
					if (error->pending()) {
						stack->pushLong(0);
						break;
					}
					if (!sum.isint && std::isinf(sum.d)) {
						error->q(ERROR_INFINITY);
						sum = matFloat(0.0);
					}
					if (sum.isint) stack->pushLong(sum.i); else stack->pushDouble(sum.d);
				}
				break;

				case OP_CROSS: {
					// the cross product.  Three elements each gives the vector
					// product, shaped like the left operand; two elements each
					// gives the single number that is the z of the three
					// dimensional answer - a torque, a winding direction, or
					// which side of a line a point falls, which is what a two
					// dimensional program actually wants.
					DataElement *b = stack->popDEborrow();		// DO NOT RELEASE
					DataElement *a = stack->popDEborrow();		// DO NOT RELEASE
					if (!vecOperand(a) || !vecOperand(b)) {
						stack->pushLong(0);
						break;
					}
					const int n = vecSize(a);
					if (vecSize(b)!=n) {
						error->q(ERROR_VECDIM);
						stack->pushLong(0);
						break;
					}
					if (n!=2 && n!=3) {
						error->q(ERROR_CROSSDIM);
						stack->pushLong(0);
						break;
					}
					// both operands are read out in full, and the shape of the
					// answer noted, before anything is pushed - a push may move
					// the stack out from under the borrowed elements
					std::vector<MatNum> x, y;
					if (!vecRead(a, x, convert) || !vecRead(b, y, convert)) {
						stack->pushLong(0);
						break;
					}
					const int rows = a->arr->xdim;
					const int cols = a->arr->ydim;

					if (n==2) {
						MatNum z = matNumSub(matNumMul(x[0], y[1]), matNumMul(x[1], y[0]));
						if (!z.isint && std::isinf(z.d)) {
							error->q(ERROR_INFINITY);
							z = matFloat(0.0);
						}
						if (z.isint) stack->pushLong(z.i); else stack->pushDouble(z.d);
						break;
					}

					MatNum c[3] = {
						matNumSub(matNumMul(x[1], y[2]), matNumMul(x[2], y[1])),
						matNumSub(matNumMul(x[2], y[0]), matNumMul(x[0], y[2])),
						matNumSub(matNumMul(x[0], y[1]), matNumMul(x[1], y[0]))
					};
					bool infinite = false;
					for (int k=0; k<3; k++) {
						if (!c[k].isint && std::isinf(c[k].d)) {
							infinite = true;
							c[k] = matFloat(0.0);
						}
					}
					// built straight into the slot it is pushed onto, so the
					// answer is never copied
					stack->pushUnassigned();
					DataElement *r = stack->peekDE(0);			// DONT RELEASE
					r->arrayDim(rows, cols, false);
					if (DataElement::getError()) {
						error->q(DataElement::getError(true));
						break;
					}
					for (int k=0; k<3; k++) matPut(&r->arr->data[k], c[k]);
					if (infinite) error->q(ERROR_INFINITY);
				}
				break;

				case OP_NORM: {
					// the length of a vector.  Always a float - a square root
					// is not a whole number except by accident.
					DataElement *v = stack->popDEborrow();		// DO NOT RELEASE
					if (!vecOperand(v)) {
						stack->pushLong(0);
						break;
					}
					const int n = vecSize(v);
					double sum = 0.0;
					for (int k=0; k<n; k++) {
						const double e = vecGet(v, k, convert).f();
						sum += e * e;
					}
					if (error->pending()) {
						stack->pushLong(0);
						break;
					}
					const double len = sqrt(sum);
					if (std::isinf(len)) {
						error->q(ERROR_INFINITY);
						stack->pushDouble(0.0);
						break;
					}
					stack->pushDouble(len);
				}
				break;

				case OP_UNIT: {
					// the same vector scaled to length one, in the shape it
					// came in.  Every element is a float, for the same reason
					// NORM is.
					DataElement *v = stack->popDEborrow();		// DO NOT RELEASE
					if (!vecOperand(v)) {
						stack->pushLong(0);
						break;
					}
					const int n = vecSize(v);
					std::vector<double> e(n);
					double sum = 0.0;
					for (int k=0; k<n; k++) {
						e[k] = vecGet(v, k, convert).f();
						sum += e[k] * e[k];
					}
					if (error->pending()) {
						stack->pushLong(0);
						break;
					}
					const double len = sqrt(sum);
					if (len==0.0) {
						error->q(ERROR_VECZERO);
						stack->pushLong(0);
						break;
					}
					if (std::isinf(len)) {
						error->q(ERROR_INFINITY);
						stack->pushLong(0);
						break;
					}
					const int rows = v->arr->xdim;
					const int cols = v->arr->ydim;
					stack->pushUnassigned();
					DataElement *r = stack->peekDE(0);			// DONT RELEASE
					r->arrayDim(rows, cols, false);
					if (DataElement::getError()) {
						error->q(DataElement::getError(true));
						break;
					}
					for (int k=0; k<n; k++) matPut(&r->arr->data[k], matFloat(e[k] / len));
				}
				break;

				case OP_LIST2MAP: {
					// pop a list of values off of stack and push
					// it back on as a single DataElement with the data as a map
					// remember this is not associated with variable
					const int n = stack->popInt();
					
					DataElement *edest = new DataElement();			// RELEASE
					edest->mapDim();

					for(int i = 0; i<n; i++) {
						DataElement *e = stack->popDE();			// RELEASE
						QString key = stack->popQString();
						edest->mapSetData(key, e);
						delete e;
					}
					stack->pushDE(edest);
					delete edest;
				}
				break;
				
				case OP_STACKSAVE: {
					// pop an element from the current stack and push to the "savestack"
					DataElement *de = stack->popDE();			// RELEASE
					savestack->pushDE(de);
					delete de;
				}
				break;
				
				case OP_STACKUNSAVE: {
					// pop an element from the "savestack" and push to the program's main stack
					DataElement *de = savestack->popDE();			// RELEASE
					stack->pushDE(de);
					delete de;
				}
				break;
				
				case OP_LJUST: {
					QString fill = stack->popQString();
					int k = stack->popInt();
					stack->pushQString(stack->popQString().leftJustified(k, fill.at(0)));
				}
				break;
				
				case OP_RJUST: {
					QString fill = stack->popQString();
					int k = stack->popInt();
					stack->pushQString(stack->popQString().rightJustified(k, fill.at(0)));
				}
				break;
				
				case OP_ROUND: {
					int places = stack->popInt();
					double v = stack->popDouble();
					double offset=pow(10,places);
					stack->pushDouble(round(v*offset)/offset);
				}
				break;
				
				case OP_ARRAYBASE: {
					int base = stack->popInt();
					if (base==0||base==1) {
						arraybase = base;
					} else {
						error->q(ERROR_ZEROORONE);
					}
				}
				break;

				case OP_GETARRAYBASE: {
					stack->pushInt(arraybase);
				}
				break;

				case OP_NEXT: {
					forframe *temp = forstack;
					forframe *prev = NULL;

					// search for FOR in stack
					// this is done because of anti-spaghetti code
					while(temp && op!=temp->nextAddr){
						prev=temp;
						temp=temp->next;
					}

					if (!temp) {
						error->q(ERROR_NEXTNOFOR);
					} else {
						switch (temp->forFrameType) {
							case FORFRAMETYPE_INT:
							{
								const qint64 val = convert->getLong(variables->getData(temp->forVarnum))+temp->intStep;
								variables->setData(temp->forVarnum, val);
								watchvariable(debugMode, temp->forVarnum);
								if ((temp->intStep > 0 && val <= temp->intEnd) ||
									(temp->intStep < 0 && val >= temp->intEnd) ||
									(temp->intStep==0 && temp->intStart < temp->intEnd && val <= temp->intEnd) ||
									(temp->intStep==0 && temp->intStart > temp->intEnd && val >= temp->intEnd) ||
									(temp->intStep==0 && temp->intStart == temp->intEnd && val == temp->intEnd)
									){
									op = temp->forAddr;
								} else {
									if(prev){
										prev->next=temp->next;
									}else{
										forstack = temp->next;
									}
									delete temp;
								}
							}
							break;
							case FORFRAMETYPE_FLOAT:
							{
						
								const double val = convert->getFloat(variables->getData(temp->forVarnum))+temp->floatStep;
								variables->setData(temp->forVarnum, val);
								watchvariable(debugMode, temp->forVarnum);
								if ((temp->floatStep > 0.0 && convert->compareFloats(val, temp->floatEnd)!=1) || 
									(temp->floatStep < 0.0 && convert->compareFloats(val, temp->floatEnd)!=-1) ||
									(temp->floatStep==0 && temp->floatStart < temp->floatEnd && val <= temp->floatEnd) ||
									(temp->floatStep==0 && temp->floatStart > temp->floatEnd && val >= temp->floatEnd) ||
									(temp->floatStep==0 && temp->floatStart == temp->floatEnd && val == temp->floatEnd)
									){
									op = temp->forAddr;
								} else {
									if(prev){
										prev->next=temp->next;
									}else{
										forstack = temp->next;
									}
									delete temp;
								}
							}
							break;
							case FORFRAMETYPE_FOREACH_ARRAY:
							{
								temp->arrayIter++;
								if (temp->arrayIter != temp->arrayIterEnd) {
									// set variable to this element
									variables->setData(temp->forVarnum, &*temp->arrayIter);
									watchvariable(debugMode, temp->forVarnum);
									// loop again
									op = temp->forAddr;
								} else {
									// done with loop
									if(prev){
										prev->next=temp->next;
									}else{
										forstack = temp->next;
									}
									delete temp->foreach_de;
									delete temp;
								}
							}
							break;
							case FORFRAMETYPE_FOREACH_MAP:
							{
								temp->mapIter++;
								if (temp->mapIter != temp->mapIterEnd) {
									// set variable1 to the next key
									variables->setData(temp->forVarnum, QString::fromStdString((std::string) (temp->mapIter->first)));
									watchvariable(debugMode, temp->forVarnum);
									// set variable2 to value
									if (temp->forVarnumValue !=-1) {
										variables->setData(temp->forVarnumValue, temp->mapIter->second);
										watchvariable(debugMode, temp->forVarnumValue);
									}
									// loop again
									op = temp->forAddr;
								} else {
									// done with loop
									if(prev){
										prev->next=temp->next;
									}else{
										forstack = temp->next;
									}
									delete temp->foreach_de;
									delete temp;
								}
							}
							break;
						}
					}
				}
				break;


				// Files: the work is done in Interpreter_files.cpp
				case OP_OPEN:
				case OP_OPENFILEDIALOG:
				case OP_SAVEFILEDIALOG:
				case OP_READLINE:
				case OP_READBYTE:
				case OP_EOF:
				case OP_WRITE:
				case OP_WRITELINE:
				case OP_WRITEBYTE:
				case OP_CLOSE:
				case OP_RESET:
				case OP_SIZE:
				case OP_EXISTS:
				case OP_SEEK:
				case OP_CHANGEDIR:
				case OP_CURRENTDIR:
				case OP_KILL:
				case OP_DIR:
				case OP_FREEFILE:
				case OP_MKDIR:
					execFileOp(opcode);
					break;

				// Sound: the work is done in Interpreter_sound.cpp
				case OP_SOUND:
				case OP_SOUNDPLAY:
				case OP_SOUNDPLAYER:
				case OP_SOUNDLOAD:
				case OP_SOUNDLOADRAW:
				case OP_SOUNDPAUSE:
				case OP_SOUNDSTOP:
				case OP_SOUNDPLAYEROFF:
				case OP_SOUNDSYSTEM:
				case OP_SOUNDSAMPLERATE:
				case OP_SOUNDWAIT:
				case OP_SOUNDNOHARMONICS:
				case OP_SOUNDHARMONICS:
				case OP_SOUNDNOENVELOPE:
				case OP_SOUNDENVELOPE:
				case OP_SOUNDWAVEFORM:
				case OP_SOUNDSEEK:
				case OP_SOUNDVOLUME:
				case OP_SOUNDLOOP:
				case OP_SOUNDID:
				case OP_SOUNDPOSITION:
				case OP_SOUNDFADE:
				case OP_SOUNDLENGTH:
				case OP_SOUNDSTATE:
				case OP_VOLUME:
				case OP_SAY:
				case OP_WAVPLAY:
				case OP_WAVSTOP:
				case OP_WAVWAIT:
				case OP_WAVLENGTH:
				case OP_WAVPAUSE:
				case OP_WAVPOS:
				case OP_WAVSEEK:
				case OP_WAVSTATE:
					execSoundOp(opcode);
					break;

				// Graphics: the work is done in Interpreter_graphics.cpp
				case OP_SETCOLOR:
				case OP_RGB:
				case OP_HSV:
				case OP_PIXEL:
				case OP_GETCOLOR:
				case OP_GETSLICE:
				case OP_PUTSLICE:
				case OP_LINE:
				case OP_ROUNDEDRECT:
				case OP_RECT:
				case OP_POLY:
				case OP_STAMP:
				case OP_CIRCLE:
				case OP_ELLIPSE:
				case OP_IMGLOAD:
				case OP_TEXT:
				case OP_TEXTBOX:
				case OP_TEXTBOXHEIGHT:
				case OP_TEXTBOXWIDTH:
				case OP_FONT:
				case OP_CLG:
				case OP_PLOT:
				case OP_WINDOW:
				case OP_FASTGRAPHICS:
				case OP_GRAPHSIZE:
				case OP_GRAPHWIDTH:
				case OP_GRAPHHEIGHT:
				case OP_REFRESH:
				case OP_MOUSEX:
				case OP_MOUSEY:
				case OP_MOUSEB:
				case OP_CLICKCLEAR:
				case OP_CLICKX:
				case OP_CLICKY:
				case OP_CLICKB:
				case OP_IMGSAVE:
				case OP_TEXTHEIGHT:
				case OP_TEXTWIDTH:
				case OP_ARC:
				case OP_CHORD:
				case OP_PIE:
				case OP_PENWIDTH:
				case OP_GETPENWIDTH:
				case OP_GETBRUSHCOLOR:
				case OP_PRINTEROFF:
				case OP_PRINTERON:
				case OP_PRINTERPAGE:
				case OP_PRINTERCANCEL:
				case OP_IMAGELOAD:
				case OP_IMAGENEW:
				case OP_IMAGECOPY:
				case OP_IMAGECROP:
				case OP_IMAGEAUTOCROP:
				case OP_IMAGERESIZE:
				case OP_IMAGESETPIXEL:
				case OP_IMAGEWIDTH:
				case OP_IMAGEHEIGHT:
				case OP_IMAGEPIXEL:
				case OP_IMAGEDRAW:
				case OP_IMAGEFLIP:
				case OP_IMAGEROTATE:
				case OP_IMAGESMOOTH:
				case OP_IMAGECENTERED:
				case OP_UNLOAD:
				case OP_IMAGETRANSFORMED:
				case OP_SETGRAPH:
					execGraphicsOp(opcode);
					break;

				// Sprites: the work is done in Interpreter_sprites.cpp
				case OP_SPRITEPOLY:
				case OP_SPRITETEXT:
				case OP_SPRITEDIM:
				case OP_SPRITELOAD:
				case OP_SPRITESLICE:
				case OP_SPRITEMOVE:
				case OP_SPRITEPLACE:
				case OP_SPRITEHIDE:
				case OP_SPRITESHOW:
				case OP_SPRITECOLLIDE:
				case OP_SPRITEX:
				case OP_SPRITEY:
				case OP_SPRITEH:
				case OP_SPRITEW:
				case OP_SPRITEV:
				case OP_SPRITER:
				case OP_SPRITES:
				case OP_SPRITEO:
					execSpriteOp(opcode);
					break;

				// Text output: the work is done in Interpreter_textoutput.cpp
				case OP_CLS:
				case OP_INPUT:
				case OP_KEY:
				case OP_KEYPRESSED:
				case OP_PRINT:
				case OP_LOCATE:
				case OP_TEXTCOLOR:
				case OP_TEXTFONT:
				case OP_TEXTCOL:
				case OP_TEXTROW:
				case OP_TEXTBACKGROUND:
				case OP_TEXTSCREEN:
				case OP_TEXTCHAR:
					execTextOutputOp(opcode);
					break;

				// Database: the work is done in Interpreter_database.cpp
				case OP_DBOPEN:
				case OP_DBCLOSE:
				case OP_DBEXECUTE:
				case OP_DBOPENSET:
				case OP_DBCLOSESET:
				case OP_DBROW:
				case OP_DBINT:
				case OP_DBFLOAT:
				case OP_DBNULL:
				case OP_DBSTRING:
				case OP_FREEDB:
				case OP_FREEDBSET:
					execDatabaseOp(opcode);
					break;

				// Network: the work is done in Interpreter_network.cpp
				case OP_NETLISTEN:
				case OP_NETCONNECT:
				case OP_NETREAD:
				case OP_NETWRITE:
				case OP_NETCLOSE:
				case OP_NETDATA:
				case OP_NETADDRESS:
				case OP_FREENET:
					execNetworkOp(opcode);
					break;

				// System and IDE: the work is done in Interpreter_system.cpp
				case OP_SYSTEM:
				case OP_SETSETTING:
				case OP_GETSETTING:
				case OP_PORTOUT:
				case OP_PORTIN:
				case OP_OSTYPE:
				case OP_EDITVISIBLE:
				case OP_GRAPHVISIBLE:
				case OP_OUTPUTVISIBLE:
				case OP_MAINTOOLBARVISIBLE:
				case OP_GRAPHTOOLBARVISIBLE:
				case OP_OUTPUTTOOLBARVISIBLE:
				case OP_MAXIMIZE:
				case OP_ALERT:
				case OP_CONFIRM:
				case OP_PROMPT:
				case OP_GETCLIPBOARDIMAGE:
				case OP_GETCLIPBOARDSTRING:
				case OP_SETCLIPBOARDIMAGE:
				case OP_SETCLIPBOARDSTRING:
					execSystemOp(opcode);
					break;

			}
		}
		break;

		default: {
			//emit(outputReady("optype=" + QString::number(optype(currentop)) + " op=" + QString::number(currentop,16) + QStringLiteral(".\n"));
			emit(outputReady(tr("Error in bytecode during label referencing at line ") + QString::number(currentLine) + QStringLiteral(".\n")));
			return -1;
		}
		break;
	}
	
	// do checks for object unhandled errors
	if (Stack::getError()) error->q(Stack::getError(true));
	if (Variables::getError()) error->q(Variables::getError(true));
	if (Convert::getError()) error->q(Convert::getError(true));
	if (DataElement::getError()) error->q(DataElement::getError(true));

	goto nextop;
}
