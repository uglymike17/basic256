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


#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <qglobal.h>


#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>



#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QFileDialog>
#include <QClipboard>
#include <QFileSystemWatcher>

#include "BasicDock.h"
#include "BasicWidget.h"
#include "BasicOutput.h"
#include "BasicEdit.h"
#include "BasicGraph.h"
#include "VariableWin.h"
#include "PreferencesWin.h"
#include "RunController.h"
#include "EditSyntaxHighlighter.h"
#include "Settings.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
  	MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring, int guistate);
	~MainWindow();
	void ifGuiStateRun();
    void ifGuiStateClose(bool ok);
	void closeEvent(QCloseEvent *);
    void setRunState(int);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    bool loadFile(QString);

    

	// Main IU Widgets
	BasicWidget * editwin_widget;
	BasicWidget * outwin_widget;
	BasicWidget * graphwin_widget;
	BasicWidget * varwin_widget;

	// file menu and choices
	QMenu * filemenu;
    QMenu * filemenu_recentfiles;
    QAction * filemenu_new_act;
	QAction * filemenu_open_act;
	QAction * filemenu_save_act;
    QAction * filemenu_saveas_act;
    QAction * filemenu_saveall_act;
    QAction * filemenu_print_act;
    QAction * filemenu_exit_act;
    QAction * filemenu_close_act;
    QAction * filemenu_closeall_act;

    // recent files submenu
    QAction * recentfiles_act[SETTINGSGROUPHISTN];
    QAction * recentfiles_empty_act;

	// edit menu and choices
	QMenu * editmenu;
	QAction *undoact;
	QAction *redoact;
	QAction *cutact;
	QAction *copyact;
	QAction *pasteact;
	QAction *selectallact;
	QAction *findact;
	QAction *findagain;
	QAction *replaceact;
	QAction *beautifyact;
	QAction *prefact;

	// view menu
	QMenu *viewmenu;
	QAction *editwin_visible_act;
	QAction *outwin_visible_act;
	QAction *graphwin_visible_act;
	QAction *varwin_visible_act;
	QAction *edit_whitespace_act;
	QAction *edit_wrap_act;
	QAction *graph_grid_visible_act;
	QAction *fontact;
	QAction *main_toolbar_visible_act;
	QAction *outwin_toolbar_visible_act;
	QAction *graphwin_toolbar_visible_act;
    QMenu * viewmenu_zoom;
    QActionGroup *viewmenu_zoom_group;
    QAction *viewmenu_zoom_1_4;
    QAction *viewmenu_zoom_1_2;
    QAction *viewmenu_zoom_1_1;
    QAction *viewmenu_zoom_2_1;
    QAction *viewmenu_zoom_3_1;
    QAction *viewmenu_zoom_4_1;

	// run menu
	QMenu *runmenu;
	QAction * runact;
	QAction * debugact;
	QAction * stepact;
	QAction * bpact;
    QAction * stopact;
    QAction * clearbreakpointsact;

    // window menu
    QMenu *windowmenu;

    // help menu
    QAction * onlinehact;
    QAction * docact;
    QAction * helpthis;
    QAction * checkupdate;

    RunController *rc;
    EditSyntaxHighlighter * editsyntax;
    QFont editorFont;


	QString localecode;
	QLocale *locale;
	
signals:
    void setEditorRunState(int);
    void saveAllStep(int);

public slots:
  void updateStatusBar(QString);
  void updateWindowTitle(BasicEdit *);
  void newProgram();
  void updateEditorButtons();
#ifndef ANDROID
  void checkForUpdate(void);
#endif

private:
	BasicDock * outwin_dock;
	BasicDock * graphwin_dock;
	QScrollArea * graph_scroll;
	BasicDock * varwin_dock;
    QTabWidget *editwintabs;


	QToolBar *main_toolbar;
    
	// SEE GUISTATE* Constants
	void configureGuiState();
    bool autoCheckForUpdate;
	
	void loadCustomizations();
	void saveCustomizations();
    BasicEdit* newEditor(QString title);
    int untitledNumber;
    int runState;
    QFileSystemWatcher *fileSystemWatcher;

#ifndef ANDROID
    QNetworkRequest request;
    QNetworkAccessManager *manager;
#endif

	// void pointer to the run controller
	// can't specify type because of circular reference
	//void *rcvoidpointer;		

private slots:
    void updateRecent();
    void updateWindowMenu();
    void emptyRecent();
    void addFileToRecentList(QString);
    void about();
    void openRecent();
    void dialogFontSelect();
    void activeEditorPrint();
    void activeEditorSaveProgram();
    void activeEditorSaveAsProgram();
    void activeEditorUndo();
    void activeEditorRedo();
    void activeEditorCut();
    void activeEditorCopy();
    void activeEditorPaste();
    void activeEditorSelectAll();
    void activeEditorBeautifyProgram();
    void activeEditorClearBreakPoints();
    void currentEditorTabChanged(int);
    void closeEditorTab(int);
    void loadProgram();
    void activeEditorCloseTab();
    bool closeAllPrograms();
    void setCurrentEditorTab(BasicEdit*);
    void saveAll();
    void updateBreakPointsAction();
    void zoomGroupActionEvent(QAction*);

#ifndef ANDROID
    void sourceforgeReplyFinished(QNetworkReply* reply);
#endif
};

#endif
