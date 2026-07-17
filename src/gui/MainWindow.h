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

#include <functional>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>



#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtWidgets/QMessageBox>
#include <QtGui/QShortcut>
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
#include "Settings.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
  	MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring, int guistate, bool fullscreen = false);
	~MainWindow();
	void ifGuiStateRun();
    void ifGuiStateClose(bool ok);
	void closeEvent(QCloseEvent *);
    void setRunState(int);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    bool loadFile(QString);
#ifdef Q_OS_WASM
    void loadFileContent(QString fileName, const QByteArray &content);
    void hidePlayerChrome();
#endif
    void resizeToFitGraph(int canvasW, int canvasH);

    

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
	QAction * filemenu_openexample_act;
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
    QFont editorFont;
    // false => editorFont is derived from the system font on every launch (so it
    // tracks the system text size, like the menus); true => the user picked one in
    // Options > Font and it is pinned. See MainWindow::defaultEditorFont().
    bool fontUserSet = false;
    static QFont defaultEditorFont();


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
  void checkForUpdate(void);

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
    bool guiFullScreen;
	
	void loadCustomizations();
	void saveCustomizations();
    void finishCloseAllPrograms(bool doit, std::function<void(bool)> onDone);
    BasicEdit* newEditor(QString title);
    int untitledNumber;
    int runState;
    // Set just before calling qApp->quit() from closeEvent()'s async completion
    // callback. QApplication's handling of the resulting QEvent::Quit calls
    // closeAllWindows(), which re-invokes closeEvent() on this same window
    // *before* the event loop actually exits -- without this flag that
    // re-entrant call would ignore() the event again (same as the original,
    // not-yet-confirmed call), which makes closeAllWindows() report failure,
    // which makes Qt abort the quit, leaving us to schedule another
    // qApp->quit() and loop forever. Once set, closeEvent() just accepts.
    bool quitConfirmed;
    QFileSystemWatcher *fileSystemWatcher;

    QNetworkRequest request;
    QNetworkAccessManager *manager;

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
    void unwatchFile(QString);
    void watchFile(QString);
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
    void openExample();
    void activeEditorCloseTab();
    void closeAllPrograms(std::function<void(bool)> onDone);
    void closeAllProgramsSlot();
    void setCurrentEditorTab(BasicEdit*);
    void saveAll();
    void updateBreakPointsAction();
    void zoomGroupActionEvent(QAction*);

    void sourceforgeReplyFinished(QNetworkReply* reply);
};

#endif
