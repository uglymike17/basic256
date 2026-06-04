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

#ifndef __INTERPRETER_H
#define __INTERPRETER_H

#include <QPixmap>
#include <QImage>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QTime>
#include <stdio.h>
#include <cmath>
#include <dirent.h>
#include "BasicGraph.h"
#include "Constants.h"
#include "DataElement.h"
#include "Error.h"
#include "Convert.h"
#include "Stack.h"
#include "Variables.h"
#include "Sound.h"
#include "Sleeper.h"
#include "BasicDownloader.h"

#include <QElapsedTimer>
#include <QDebug>
#include <QProcess>


#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrinterInfo>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

#ifndef ANDROID
    // includes for all ports EXCEPT android
    #include <QSerialPort>
#endif

enum run_status {R_STOPPED, R_RUNNING, R_STOPING, R_PRESTOPING};

#define NUMFILES 8
#define NUMSOCKETS 8
#define NUMDBCONN 8
#define NUMDBSET 8

#define STRINGMAXLEN 16777216

#define FILEWRITETIMEOUT		1			// on a file/serial write wait up to MS for the write to complete
#define FILEREADTIMEOUT			1			// on a file/serial read wait up to MS for data to be there
#define SERIALREADBUFFERSIZE	1024		// size of openserial read buffer

#define FORFRAMETYPE_INT 0
#define FORFRAMETYPE_FLOAT 1
#define FORFRAMETYPE_FOREACH_ARRAY 2
#define FORFRAMETYPE_FOREACH_MAP 3


struct byteCodeData
{
    unsigned int size;
    void *data;
};

// used by function calls, subroutine calls, and gosubs for return location
// used also by onerror
// used to track nested on-error and try/catch definitions
class addrStack {
public:
    addrStack(){
        size=0;
        pointer=0;
        stack.reserve(16);
    };
    ~addrStack(){};
    void push(int* address){
        if(pointer>=size) grow();
        stack[pointer]=address;
        pointer++;
    };
    int* pop(){
        if(pointer==0) return NULL;
        pointer--;
        return stack[pointer];
    };
    int* peek(){
        if(pointer==0) return NULL;
        return stack[pointer-1];
    };
    void drop(){
        if(pointer>0) pointer--;
    };
    int count(){
        return pointer;
    };
private:
    int size;
    int pointer;
    std::vector<int*> stack;
    void grow(){
        size=size+8;
        stack.resize(size);
    };
};

struct trycatchframe {
    trycatchframe *next;
    int *catchAddr;
    int recurseLevel;
    int stackSize;
};

// structure for the nested for statements
// if useInt then make loop integer safe
struct forframe {
    forframe *next;
    int *forAddr;   //FOR address
    int *nextAddr;  //NEXT address
    int forFrameType;		//0=integer, 1=float, 2=foreache if -
    int forVarnum;
    int forVarnumValue;		// -1 if not used (used to get map value in for each)
    double floatStart;
    double floatEnd;
    double floatStep;
    long intStart;
    long intEnd;
    long intStep;
    DataElement* foreach_de;		// copy of data used for the foreach
    std::vector<DataElement*>::iterator arrayIter;
    std::vector<DataElement*>::iterator arrayIterEnd;
    std::map<std::string, DataElement*>::iterator mapIter;
    std::map<std::string, DataElement*>::iterator mapIterEnd;
};

typedef struct {
    bool visible;
    double x;
    double y;
    double r;	// rotate
    double s;	// scale
    double o;	// opacity
    QImage *image;
    QImage *transformed_image;
    QRect position;
    bool changed;
    bool was_printed;
    QRect last_position;
} sprite;

class Interpreter : public QThread
{
	Q_OBJECT;
	public:
		Interpreter(QLocale*);
		~Interpreter();
		int compileProgram(char *);
		void initialize();
		bool isRunning();
		bool isStopped();
		bool isStopping();
		void setStatus(run_status);
		void setInputString(QString);	// used to return string vlues from runcontroller (into inputString)
		void cleanup();
		void run();
		int debugMode;					// 0=normal run, 1=step execution, 2=run to breakpoint
		QList<int> *debugBreakPoints;	// map of line numbers where break points ( pointer to breakpoint list in basicedit)
		int returnInt;					// return value from runcontroller emit
		QImage returnImage;				// return value from runcontroller emit
		int settingsAllowPort;
		int settingsAllowSystem;

	public slots:
		int execByteCode();
		void runHalted();

	signals:
		void debugNextStep();
		void fastGraphics();
		//void stopRun();
		void stopRunFinalized(bool);
		void goutputReady();
		void outputReady(QString);
		void outputError(QString);
		void outputTextAt(int, int, QString);
		void getInput();
		void outputClear();
		void getKey();
		void playSounds(int, int*);
		void setVolume(int);
		void speakWords(QString);
		void goToLine(int);
		void seekLine(int);
		void varWinAssign(Variables**, int, int);
		void varWinAssign(Variables**, int, int, int, int);
		void varWinAssign(Variables**, int, int, QString);
		void varWinDropLevel(int);
		void varWinDimArray(Variables**, int, int, int);
		void resizeGraphWindow(int, int, qreal);
		void mainWindowsVisible(int, bool);
		void dialogAlert(QString);
		void dialogConfirm(QString, int);
		void dialogPrompt(QString, QString);
		void dialogOpenFileDialog(QString, QString, QString);
		void dialogSaveFileDialog(QString, QString, QString);
		void dialogAllowPortInOut(QString);
		void dialogAllowSystem(QString);
		void playSound(QString, bool);
		void playSound(std::vector<std::vector<double>>, bool);
		void loadSoundFromArray(QString, QByteArray*);
		void soundStop(int);
		void soundPlay(int);
		void soundFade(int, double, int, int);
		void soundVolume(int, double);
		//void soundExit();
		void soundPlayerOff(int);
		void soundSystem(int);
		void getClipboardImage();
		void getClipboardString();
		void setClipboardImage(QImage);
		void setClipboardString(QString);
		

	private:
		QLocale *locale;
		Sleeper *sleeper;
		BasicDownloader *downloader;
		//int optype(int op);
		QString opname(int);
		void waitForGraphics();
		void printError();
		int netSockClose(int);
		void netSockCloseAll();
		Variables *variables;
		Stack *stack;
		Stack *savestack;
		bool isError; //flag set if program stops because of an error
		Convert *convert;
		QIODevice **filehandle;
		int *filehandletype;		// 0=QFile (normal), 1=QFile (binary), 2=QSerialPort
		int *op;
		addrStack *callstack;
		addrStack *onerrorstack;
		int* onstopaddr;
		trycatchframe *trycatchstack; // used to track nested try/catch definitions
		void decreaserecurse();
		forframe *forstack;                     // stack FOR/NEXT for current recurse level
		std::vector <forframe*> forstacklevel;  // stack FOR/NEXT for each recurse level
		int forstacklevelsize;                  // size for forstacklevel stack
		run_status status;
		bool fastgraphics;
		QString inputString;        // input string from user
		int inputType;				// data type to convert the input into
		double double_random_max;
		int currentLine;
		void clearsprites();
		void update_sprite_screen();
		void sprite_prepare_for_new_content(int);
		void force_redraw_all_sprites_next_time();
		bool sprite_collide(int, int, bool);
		sprite *sprites;
		int nsprites;
		void closeDatabase(int);
		int arraybase;			// 0 for 0..n-1, 1 for 1 to n array indexing
		// watch... functions trigger the variablewatch window to display
		void watchvariable(bool, int);
		void watchvariable(bool, int, int, int);
		void watchvariable(bool, int, QString);
		void watchdecurse(bool);
		
		void runLoop();

		int listensockfd;				// temp socket used in netlisten
		int netsockfd[NUMSOCKETS];

		DIR *directorypointer;		// used by DIR function
		QTime runtimer;				// used by MSEC function
		//SoundSystem *sound;
		int includeFileNumber;
		bool regexMinimal;			// flag to tell QRegExp to be greedy (false) or minimal (true)

		bool printing;
		QPrinter *printdocument;

		QPainter *painter;
		bool painter_pen_need_update;
		bool painter_brush_need_update;
		bool painter_last_compositionModeClear;
		QColor painter_brush_color; //last color value for comparison
		QColor painter_pen_color; //last color value for comparison
		void setGraph(QString id);
		QString drawto;
		bool setPainterTo(QPaintDevice *destination);
		QPen drawingpen;
		QBrush drawingbrush;
		int CompositionModeClear;
		int PenColorIsClear;
		bool drawingOnScreen;

		QFont font;
		QString defaultfontfamily;
		int defaultfontpointsize;
		int defaultfontweight;
		bool defaultfontitalic;

		bool painter_font_need_update;
		bool painter_custom_font_flag;

		QSqlQuery *dbSet[NUMDBCONN][NUMDBSET];		// allow NUMDBSET number of sets on a database connection

		int mediaplayer_id_legacy;

		QMap <QString, QImage*> images;
		int lastImageId;
		bool imageSmooth;

		int settingsDebugSpeed;
		bool settingsAllowSetting;
		int settingsSettingsAccess;
		int settingsSettingsMax;
		QString programName;
		int settingsPrinterResolution;
		int settingsPrinterPrinter;
		int settingsPrinterPaper;
		QString settingsPrinterPdfFile;
		int settingsPrinterOrient;
		QMap<QString, QMap<QString, QString>> fakeSettings;
		QProcess *sys;

		QString originalPath;				// used to restore IDE path afrer it may be changed at run time
};


#endif
