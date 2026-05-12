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



#include <iostream>
#include <stdio.h>
#include <unordered_set>

#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <QDesktopServices> 
 
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QDesktopWidget>

#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"
#include "BasicDock.h"
#include "BasicIcons.h"
#include "BasicKeyboard.h"

// global mymutexes and timers
QMutex* mymutex;
QMutex* mydebugmutex;
QWaitCondition* waitCond;
QWaitCondition* waitDebugCond;
BasicIcons *basicIcons;

// the three main components of the UI (define globally)
MainWindow * mainwin;
BasicEdit * editwin;
BasicOutput * outwin;
BasicGraph * graphwin;
VariableWin * varwin;
BasicKeyboard * basicKeyboard;

//global GUI state
int guiState;


MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring, int guistate)
    :	QMainWindow(parent, f) {

    localecode = localestring;
	locale = new QLocale(localecode);
    guiState = guistate;
    mainwin = this;
    setAcceptDrops(true);
    untitledNumber = 1;
    runState = RUNSTATESTOP;
    editwin=NULL;
    basicIcons = new BasicIcons();
    basicKeyboard = new BasicKeyboard();


    // create the global mymutexes and waits
    mymutex = new QMutex(QMutex::NonRecursive);
    mydebugmutex = new QMutex(QMutex::NonRecursive);
    waitCond = new QWaitCondition();
    waitDebugCond = new QWaitCondition();

    setWindowIcon(basicIcons->basic256Icon);

#ifndef ANDROID
    manager = new QNetworkAccessManager(this);
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SecureProtocols);
    request.setSslConfiguration(config);
#ifdef WIN32PORTABLE
    request.setUrl(QUrl("http://sourceforge.net/projects/basic256prtbl/best_release.json"));
#else
    request.setUrl(QUrl("http://sourceforge.net/projects/kidbasic/best_release.json"));
#endif
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "App/1.0");
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sourceforgeReplyFinished(QNetworkReply*)));
#endif


	
    // Create a QTabWidget to hold multiple editors
    editwintabs = new QTabWidget(this);
    editwintabs->setMovable(guiState==GUISTATENORMAL);
    editwintabs->setTabsClosable(guiState==GUISTATENORMAL);
    editwintabs->setDocumentMode(true);



    // Basic* *win go into BasicWidget *win_widget to get menus and toolbars
    // *win_widget go into BasicDock *win_dock to create the GUI docks

    outwin = new BasicOutput();
    outwin->setObjectName( "outwin" );
    outwin_widget = new BasicWidget(QObject::tr("Text Output"), "outwin_widget", outwin);
    outwin_dock = new BasicDock();
    outwin_dock->setObjectName( "outwin_dock" );
    outwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    outwin_dock->setWidget(outwin_widget);
    outwin_dock->setWindowTitle(QObject::tr("Text Output"));
 
    graphwin = new BasicGraph();
    graphwin->setObjectName( "graphwin" );
    graph_scroll = new QScrollArea();
    graph_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    graph_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    graphwin_widget = new BasicWidget(QObject::tr("Graphics Output"), "graphwin_widget", graphwin, graph_scroll);
    graphwin_dock = new BasicDock();
    graphwin_dock->setObjectName( "graphwin_dock" );
    graphwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    graphwin_dock->setWidget(graphwin_widget);
    graphwin_dock->setWindowTitle(QObject::tr("Graphics Output"));

    varwin = new VariableWin();
    varwin->setObjectName( "varwin" );
    varwin_dock = new BasicDock();
    varwin_dock->setObjectName( "varwin_dock" );
    varwin_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    varwin_dock->setWidget(varwin);
    varwin_dock->setWindowTitle(QObject::tr("Variable Watch"));

    setCentralWidget(editwintabs);
    addDockWidget(Qt::RightDockWidgetArea, outwin_dock);
    addDockWidget(Qt::RightDockWidgetArea, graphwin_dock);
    addDockWidget(Qt::LeftDockWidgetArea, varwin_dock);
    setContextMenuPolicy(Qt::NoContextMenu);

    rc = new RunController();

    // Main window toolbar
    main_toolbar = new QToolBar();
    main_toolbar->setObjectName("main_toolbar");
    addToolBar(main_toolbar);

    // File menu
    filemenu = menuBar()->addMenu(QObject::tr("&File"));
    filemenu_new_act = filemenu->addAction(basicIcons->newIcon, QObject::tr("&New"));
    filemenu_new_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::New));
    filemenu_open_act = filemenu->addAction(basicIcons->openIcon, QObject::tr("&Open..."));
    filemenu_open_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Open));

    // Recent files menu
    filemenu_recentfiles = filemenu->addMenu(QObject::tr("Open &Recent"));
    for(int i=0;i<SETTINGSGROUPHISTN;i++){
        recentfiles_act[i] = filemenu_recentfiles->addAction(basicIcons->openIcon, QObject::tr(""));
        if(i<10)
            recentfiles_act[i]->setShortcut(Qt::Key_0 + ((i+1)%SETTINGSGROUPHISTN) + Qt::CTRL);
    }
    filemenu_recentfiles->addSeparator();
    recentfiles_empty_act = filemenu_recentfiles->addAction(basicIcons->clearIcon, QObject::tr("&Clear list"));
    updateRecent();

    // File menu - continue
    filemenu_save_act = filemenu->addAction(basicIcons->saveIcon, QObject::tr("&Save"));
    filemenu_save_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Save));
    filemenu_saveas_act = filemenu->addAction(basicIcons->saveAsIcon, QObject::tr("Save &As..."));
    filemenu_saveas_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::SaveAs));
    filemenu_saveall_act = filemenu->addAction(basicIcons->saveAllIcon, QObject::tr("Save All"));
    filemenu->addSeparator();
    filemenu_close_act = filemenu->addAction(QObject::tr("Close"));
    filemenu_close_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Close));
    filemenu_closeall_act = filemenu->addAction(QObject::tr("Close All"));
    filemenu->addSeparator();
    filemenu_print_act = filemenu->addAction(basicIcons->printIcon, QObject::tr("&Print..."));
    filemenu_print_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Print));
    filemenu->addSeparator();
    filemenu_exit_act = filemenu->addAction(basicIcons->exitIcon, QObject::tr("&Exit"));
    filemenu_exit_act->setShortcuts(QKeySequence::keyBindings(QKeySequence::Quit));


    // Edit menu
    editmenu = menuBar()->addMenu(QObject::tr("&Edit"));
    undoact = editmenu->addAction(basicIcons->undoIcon, QObject::tr("&Undo"));
    undoact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Undo));
    undoact->setEnabled(false);
    redoact = editmenu->addAction(basicIcons->redoIcon, QObject::tr("&Redo"));
    redoact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Redo));
    redoact->setEnabled(false);
    editmenu->addSeparator();
    cutact = editmenu->addAction(basicIcons->cutIcon, QObject::tr("Cu&t"));
    cutact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Cut));
    cutact->setEnabled(false);
    copyact = editmenu->addAction(basicIcons->copyIcon, QObject::tr("&Copy"));
    copyact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Copy));
    copyact->setEnabled(false);
    pasteact = editmenu->addAction(basicIcons->pasteIcon, QObject::tr("&Paste"));
    pasteact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Paste));
    editmenu->addSeparator();
    selectallact = editmenu->addAction(QObject::tr("Select &All"));
    selectallact->setShortcuts(QKeySequence::keyBindings(QKeySequence::SelectAll));
    editmenu->addSeparator();
    findact = editmenu->addAction(basicIcons->findIcon, QObject::tr("&Find..."));
    findact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Find));
    findagain = editmenu->addAction(QObject::tr("Find &Next"));
    findagain->setShortcuts(QKeySequence::keyBindings(QKeySequence::FindNext));
    replaceact = editmenu->addAction(QObject::tr("&Replace..."));
    replaceact->setShortcuts(QKeySequence::keyBindings(QKeySequence::Replace));
    editmenu->addSeparator();
    beautifyact = editmenu->addAction(QObject::tr("&Beautify"));
    editmenu->addSeparator();
    prefact = editmenu->addAction(basicIcons->preferencesIcon, QObject::tr("Preferences..."));
    //
    editmenu->addSeparator();
    editmenu->addMenu(outwin_widget->getMenu());
    editmenu->addMenu(graphwin_widget->getMenu());

    // View menuBar
    viewmenu = menuBar()->addMenu(QObject::tr("&View"));

    editwin_visible_act = viewmenu->addAction(QObject::tr("&Edit Window"));
    editwin_visible_act->setCheckable(true);
    editwin_visible_act->setChecked(SETTINGSEDITVISIBLEDEFAULT);

    outwin_visible_act = viewmenu->addAction(QObject::tr("&Text Window"));
    outwin_visible_act->setCheckable(true);
    outwin_dock->setActionCheck(outwin_visible_act);
    outwin_visible_act->setChecked(SETTINGSOUTVISIBLEDEFAULT);

    graphwin_visible_act = viewmenu->addAction(QObject::tr("&Graphics Window"));
    graphwin_visible_act->setCheckable(true);
    graphwin_dock->setActionCheck(graphwin_visible_act);
    graphwin_visible_act->setChecked(SETTINGSGRAPHVISIBLEDEFAULT);

    varwin_visible_act = viewmenu->addAction(QObject::tr("&Variable Watch Window"));
    varwin_visible_act->setCheckable(true);
    varwin_dock->setActionCheck(varwin_visible_act);
    varwin_visible_act->setChecked(SETTINGSVARVISIBLEDEFAULT);

    // Graphics Grid Lines
    viewmenu->addSeparator();
    graph_grid_visible_act = viewmenu->addAction(basicIcons->gridIcon, QObject::tr("Graphics Window Grid &Lines"));
    graph_grid_visible_act->setCheckable(true);
    graph_grid_visible_act->setChecked(SETTINGSGRAPHGRIDLINESDEFAUT);
    graphwin->slotGridLines(SETTINGSGRAPHGRIDLINESDEFAUT);

    // Graphics Zoom
    double z = graphwin->getZoom();
    viewmenu_zoom = viewmenu->addMenu(basicIcons->zoomInIcon, QObject::tr("Graphics Window &Zoom"));
    viewmenu_zoom_group = new QActionGroup(this);
    viewmenu_zoom_group->setExclusive(true);
    viewmenu_zoom_1_4 = viewmenu_zoom_group->addAction(QObject::tr("1:4 (quarter)"));
    viewmenu_zoom_1_4->setCheckable(true);
    viewmenu_zoom_1_4->setChecked(z==0.25);
    viewmenu_zoom_1_4->setData(0.25);
    viewmenu_zoom_1_2 = viewmenu_zoom_group->addAction(QObject::tr("1:2 (half)"));
    viewmenu_zoom_1_2->setCheckable(true);
    viewmenu_zoom_1_2->setChecked(z==0.5);
    viewmenu_zoom_1_2->setData(0.5);
    viewmenu_zoom_1_1 = viewmenu_zoom_group->addAction(QObject::tr("1:1 (original)"));
    viewmenu_zoom_1_1->setCheckable(true);
    viewmenu_zoom_1_1->setChecked(z==1.0);
    viewmenu_zoom_1_1->setData(1.0);
    viewmenu_zoom_2_1 = viewmenu_zoom_group->addAction(QObject::tr("2:1 (double)"));
    viewmenu_zoom_2_1->setCheckable(true);
    viewmenu_zoom_2_1->setChecked(z==2.0);
    viewmenu_zoom_2_1->setData(2.0);
    viewmenu_zoom_3_1 = viewmenu_zoom_group->addAction(QObject::tr("3:1 (triple)"));
    viewmenu_zoom_3_1->setCheckable(true);
    viewmenu_zoom_3_1->setChecked(z==3.0);
    viewmenu_zoom_3_1->setData(3.0);
    viewmenu_zoom_4_1 = viewmenu_zoom_group->addAction(QObject::tr("4:1 (quadruple)"));
    viewmenu_zoom_4_1->setCheckable(true);
    viewmenu_zoom_4_1->setChecked(z==4.0);
    viewmenu_zoom_4_1->setData(4.0);
    viewmenu_zoom->addActions(viewmenu_zoom_group->actions());



    // Editor and Output font and Editor settings
    viewmenu->addSeparator();
    fontact = viewmenu->addAction(basicIcons->fontIcon, QObject::tr("&Font..."));
    edit_whitespace_act = viewmenu->addAction(QObject::tr("Show &Whitespace Characters"));
    edit_whitespace_act->setCheckable(true);
    edit_whitespace_act->setChecked(SETTINGSEDITWHITESPACEDEFAULT);

    // Toolbars
    viewmenu->addSeparator();
    QMenu *viewtbars = viewmenu->addMenu(QObject::tr("&Toolbars"));
    main_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Main"));
    main_toolbar_visible_act->setCheckable(true);
    main_toolbar_visible_act->setChecked(SETTINGSTOOLBARVISIBLEDEFAULT);
    outwin_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Text Output"));
    outwin_toolbar_visible_act->setCheckable(true);
    outwin_toolbar_visible_act->setChecked(SETTINGSOUTTOOLBARVISIBLEDEFAULT);
    graphwin_toolbar_visible_act = viewtbars->addAction(QObject::tr("&Graphics Output"));
    graphwin_toolbar_visible_act->setCheckable(true);
    graphwin_toolbar_visible_act->setChecked(SETTINGSGRAPHTOOLBARVISIBLEDEFAULT);
 
    // Run menu
    runmenu = menuBar()->addMenu(QObject::tr("&Run"));
    runact = runmenu->addAction(basicIcons->runIcon, QObject::tr("&Run"));
    runact->setShortcut(Qt::Key_F5);
    editmenu->addSeparator();
    debugact = runmenu->addAction(basicIcons->debugIcon, QObject::tr("&Debug"));
    debugact->setShortcut(Qt::Key_F5 + Qt::CTRL);
    stepact = runmenu->addAction(basicIcons->stepIcon, QObject::tr("S&tep"));
    stepact->setShortcut(Qt::Key_F11);
    stepact->setEnabled(false);
    bpact = runmenu->addAction(basicIcons->breakIcon, QObject::tr("Run &to"));
    bpact->setShortcut(Qt::Key_F11 + Qt::CTRL);
    bpact->setEnabled(false);
    stopact = runmenu->addAction(basicIcons->stopIcon, QObject::tr("&Stop"));
    stopact->setShortcut(Qt::Key_F5 + Qt::SHIFT);
    stopact->setEnabled(false);
    runmenu->addSeparator();
    clearbreakpointsact = runmenu->addAction(basicIcons->clearIcon, QObject::tr("&Clear all breakpoints"));

    // Window menuBar
    windowmenu = menuBar()->addMenu(QObject::tr("&Window"));




    // Help menu
    QMenu *helpmenu = menuBar()->addMenu(QObject::tr("&Help"));
    onlinehact = helpmenu->addAction(basicIcons->webIcon, QObject::tr("&Online help..."));
    onlinehact->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    addAction (helpthis);
#ifndef ANDROID
    helpmenu->addSeparator();
    checkupdate = helpmenu->addAction(QObject::tr("&Check for update..."));
#endif
    helpmenu->addSeparator();
    QAction *aboutact = helpmenu->addAction(basicIcons->infoIcon, QObject::tr("&About BASIC-256..."));

    // Add actions to main window toolbar
    main_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    main_toolbar->addAction(filemenu_new_act);
    main_toolbar->addAction(filemenu_open_act);
    main_toolbar->addAction(filemenu_save_act);
    main_toolbar->addSeparator();
    main_toolbar->addAction(runact);
    main_toolbar->addAction(debugact);
    main_toolbar->addAction(stepact);
    main_toolbar->addAction(bpact);
    main_toolbar->addAction(stopact);
    main_toolbar->addSeparator();
    main_toolbar->addAction(undoact);
    main_toolbar->addAction(redoact);
    main_toolbar->addAction(cutact);
    main_toolbar->addAction(copyact);
    main_toolbar->addAction(pasteact);
	//

	loadCustomizations();
    configureGuiState();

    //Display windows and toolbars acording their final settings
    editwintabs->setVisible(editwin_visible_act->isChecked());
    graphwin_dock->setVisible(graphwin_visible_act->isChecked());
    outwin_dock->setVisible(outwin_visible_act->isChecked());
    varwin_dock->setVisible(varwin_visible_act->isChecked());
    main_toolbar->setVisible(main_toolbar_visible_act->isChecked());
    graphwin_widget->slotShowToolBar(graphwin_toolbar_visible_act->isChecked());
    outwin_widget->slotShowToolBar(outwin_toolbar_visible_act->isChecked());
    graphwin->slotGridLines(graph_grid_visible_act->isChecked());


    // connect the signals
    QObject::connect(editwintabs, SIGNAL(currentChanged(int)), this, SLOT(currentEditorTabChanged(int)));
    QObject::connect(editwintabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeEditorTab(int)));
    QObject::connect(editwin_visible_act, SIGNAL(triggered(bool)), editwintabs, SLOT(setVisible(bool)));
    QObject::connect(windowmenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu())); //tabs can be moved by user so we need to reflect the true order in menu

    QObject::connect(filemenu_print_act, SIGNAL(triggered()), this, SLOT(activeEditorPrint()));
    QObject::connect(filemenu_save_act, SIGNAL(triggered()), this, SLOT(activeEditorSaveProgram()));
    QObject::connect(filemenu_saveas_act, SIGNAL(triggered()), this, SLOT(activeEditorSaveAsProgram()));
    QObject::connect(filemenu_close_act, SIGNAL(triggered()), this, SLOT(activeEditorCloseTab()));
    QObject::connect(filemenu_closeall_act, SIGNAL(triggered()), this, SLOT(closeAllPrograms()));
    QObject::connect(undoact, SIGNAL(triggered()), this, SLOT(activeEditorUndo()));
    QObject::connect(redoact, SIGNAL(triggered()), this, SLOT(activeEditorRedo()));
    QObject::connect(cutact, SIGNAL(triggered()), this, SLOT(activeEditorCut()));
    QObject::connect(copyact, SIGNAL(triggered()), this, SLOT(activeEditorCopy()));
    QObject::connect(pasteact, SIGNAL(triggered()), this, SLOT(activeEditorPaste()));
    QObject::connect(selectallact, SIGNAL(triggered()), this, SLOT(activeEditorSelectAll()));
    QObject::connect(beautifyact, SIGNAL(triggered()), this, SLOT(activeEditorBeautifyProgram()));
    QObject::connect(clearbreakpointsact, SIGNAL(triggered()), this, SLOT(activeEditorClearBreakPoints()));

    QObject::connect(filemenu_new_act, SIGNAL(triggered()), this, SLOT(newProgram()));
    QObject::connect(filemenu_open_act, SIGNAL(triggered()), this, SLOT(loadProgram()));

    QObject::connect(filemenu_recentfiles, SIGNAL(aboutToShow()), this, SLOT(updateRecent())); //in case that another instance modify history
    for(int i=0;i<SETTINGSGROUPHISTN;i++){
        QObject::connect(recentfiles_act[i], SIGNAL(triggered()), this, SLOT(openRecent()));
    }
    QObject::connect(recentfiles_empty_act, SIGNAL(triggered()), this, SLOT(emptyRecent()));
    QObject::connect(filemenu_exit_act, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(filemenu_saveall_act, SIGNAL(triggered()), this, SLOT(saveAll()));

	QObject::connect(graphwin_toolbar_visible_act, SIGNAL(toggled(bool)), graphwin_widget, SLOT(slotShowToolBar(const bool)));
	QObject::connect(graphwin_visible_act, SIGNAL(triggered(bool)), graphwin_dock, SLOT(setVisible(bool)));

	QObject::connect(main_toolbar_visible_act, SIGNAL(toggled(bool)), main_toolbar, SLOT(setVisible(bool)));

	QObject::connect(outwin_visible_act, SIGNAL(triggered(bool)), outwin_dock, SLOT(setVisible(bool)));
	QObject::connect(outwin_toolbar_visible_act, SIGNAL(toggled(bool)), outwin_widget, SLOT(slotShowToolBar(const bool)));

	QObject::connect(varwin_visible_act, SIGNAL(triggered(bool)), varwin_dock, SLOT(setVisible(bool)));
    QObject::connect(viewmenu_zoom_group, SIGNAL(triggered(QAction*)), this, SLOT(zoomGroupActionEvent(QAction*)));



    QObject::connect(findact, SIGNAL(triggered()), rc, SLOT(showFind()));
    QObject::connect(findagain, SIGNAL(triggered()), rc, SLOT(findAgain()));
    QObject::connect(replaceact, SIGNAL(triggered()), rc, SLOT(showReplace()));
    QObject::connect(prefact, SIGNAL(triggered()), rc, SLOT(showPreferences()));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updateEditorButtons()));
    QObject::connect(fontact, SIGNAL(triggered()), this, SLOT(dialogFontSelect()));
    QObject::connect(graph_grid_visible_act, SIGNAL(toggled(bool)), graphwin, SLOT(slotGridLines(bool)));

    QObject::connect(runact, SIGNAL(triggered()), rc, SLOT(startRun()));
    QObject::connect(debugact, SIGNAL(triggered()), rc, SLOT(startDebug()));
    QObject::connect(stepact, SIGNAL(triggered()), rc, SLOT(stepThrough()));
    QObject::connect(bpact, SIGNAL(triggered()), rc, SLOT(stepBreakPoint()));
    QObject::connect(stopact, SIGNAL(triggered()), rc, SLOT(stopRun()));
    QObject::connect(runmenu, SIGNAL(aboutToShow()), this, SLOT(updateBreakPointsAction()));


    QObject::connect(onlinehact, SIGNAL(triggered()), rc, SLOT(showOnlineDocumentation()));
    QObject::connect(aboutact, SIGNAL(triggered()), this, SLOT(about()));

#ifndef ANDROID
    QObject::connect(checkupdate, SIGNAL(triggered()), this, SLOT(checkForUpdate()));
#endif
QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showOnlineContextDocumentation()));


#ifndef ANDROID
    //check for update as soon as the event loop is idle (avoid GUI freezing)
    if(autoCheckForUpdate) QTimer::singleShot(0, this, SLOT(checkForUpdate()));
#endif

    if(guiState==GUISTATENORMAL){
        fileSystemWatcher = new QFileSystemWatcher(this);
    }else{
        fileSystemWatcher=NULL;
    }
}

void MainWindow::loadCustomizations() {
	// from settings - load the customizations to the screen

	SETTINGS;
    bool v, restoreWindows;

    restoreWindows = settings.value(SETTINGSWINDOWSRESTORE, SETTINGSWINDOWSRESTOREDEFAULT).toBool();
    if(restoreWindows){
        restoreGeometry(settings.value(SETTINGSMAINGEOMETRY + QString::number(guiState)).toByteArray());
        QByteArray state = settings.value(SETTINGSMAINSTATE + QString::number(guiState)).toByteArray();
        restoreState(state);
        // edit window
        v = settings.value(SETTINGSEDITVISIBLE + QString::number(guiState), SETTINGSEDITVISIBLEDEFAULT).toBool();
        editwin_visible_act->setChecked(v);
        // graph window
        v = settings.value(SETTINGSGRAPHVISIBLE + QString::number(guiState), SETTINGSGRAPHVISIBLEDEFAULT).toBool();
        graphwin_visible_act->setChecked(v);
        // out window
        v = settings.value(SETTINGSOUTVISIBLE + QString::number(guiState), SETTINGSOUTVISIBLEDEFAULT).toBool();
        outwin_visible_act->setChecked(v);
        // var window - variable watch
        v = settings.value(SETTINGSVARVISIBLE + QString::number(guiState), SETTINGSVARVISIBLEDEFAULT).toBool();
        varwin_visible_act->setChecked(v);
    }

    // main toolbar
    v = settings.value(SETTINGSTOOLBARVISIBLE + QString::number(guiState), SETTINGSTOOLBARVISIBLEDEFAULT).toBool();
    main_toolbar_visible_act->setChecked(v);

    // edit whitespace
    v = settings.value(SETTINGSEDITWHITESPACE + QString::number(guiState), SETTINGSEDITWHITESPACEDEFAULT).toBool();
    edit_whitespace_act->setChecked(v);

    // graph toolbar and grid
    v = settings.value(SETTINGSGRAPHGRIDLINES + QString::number(guiState), SETTINGSGRAPHGRIDLINESDEFAUT).toBool();
    graph_grid_visible_act->setChecked(v);
    v = settings.value(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), SETTINGSGRAPHTOOLBARVISIBLEDEFAULT).toBool();
    graphwin_toolbar_visible_act->setChecked(v);

    // out toolbar
    v = settings.value(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), SETTINGSOUTTOOLBARVISIBLEDEFAULT).toBool();
    outwin_toolbar_visible_act->setChecked(v);

    // set initial font
    QString initialFontString = settings.value(SETTINGSFONT + QString::number(guiState),SETTINGSFONTDEFAULT).toString();
    editorFont.fromString(initialFontString);
    outwin->setFont(editorFont);

#ifndef ANDROID
    autoCheckForUpdate = (guiState==GUISTATENORMAL&&settings.value(SETTINGSCHECKFORUPDATE, SETTINGSCHECKFORUPDATEDEFAULT).toBool());
#endif
}


void MainWindow::saveCustomizations() {
    // save user customizations on close

    SETTINGS;
    settings.setValue(SETTINGSMAINGEOMETRY + QString::number(guiState), saveGeometry());
    settings.setValue(SETTINGSMAINSTATE + QString::number(guiState), saveState());

    // main
	settings.setValue(SETTINGSTOOLBARVISIBLE + QString::number(guiState), main_toolbar->isVisible());

    // edit
	settings.setValue(SETTINGSEDITVISIBLE + QString::number(guiState), editwin_visible_act->isChecked());
	settings.setValue(SETTINGSEDITWHITESPACE + QString::number(guiState), edit_whitespace_act->isChecked());

    // graph
	settings.setValue(SETTINGSGRAPHVISIBLE + QString::number(guiState), graphwin_visible_act->isChecked());
	settings.setValue(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), graphwin_widget->isVisibleToolBar());
	settings.setValue(SETTINGSGRAPHGRIDLINES + QString::number(guiState), graphwin->isVisibleGridLines());

    // out
	settings.setValue(SETTINGSOUTVISIBLE + QString::number(guiState), outwin_visible_act->isChecked());
	settings.setValue(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), outwin_widget->isVisibleToolBar());

    // var
	settings.setValue(SETTINGSVARVISIBLE + QString::number(guiState), varwin_visible_act->isChecked());

    // font
    settings.setValue(SETTINGSFONT + QString::number(guiState), editorFont.toString());

    // zoom
    settings.setValue(SETTINGSZOOM, QString::number(graphwin->getZoom()));
}

MainWindow::~MainWindow() {
    delete rc;
    delete mymutex;
    delete waitCond;
    delete outwin;
    delete graphwin;
    delete main_toolbar;
    if (locale) delete(locale);
}

void MainWindow::about() {
    QString title;
    QString message;
    SETTINGS;
    bool usefloatlocale = settings.value(SETTINGSFLOATLOCALE, SETTINGSFLOATLOCALEDEFAULT).toBool();


#ifdef ANDROID
    // android does not have webkit dialogs - make plain text
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Android");
    message = QObject::tr("BASIC-256") + QObject::tr(" Android") + "\n" +
              QObject::tr("version ") +  VERSION + QObject::tr(" - built with QT ") + QT_VERSION_STR + "\n" +
            QObject::tr("Locale Name: ") + locale->name() + QObject::tr("Decimal Point: ")+  "'" + (usefloatlocale?locale->decimalPoint():'.') + "'\n" +
              QObject::tr("Copyright &copy; 2006-2020, The BASIC-256 Team") + "\n" +
              QObject::tr("Please visit our web site at http://www.basic256.org for tutorials and documentation.") + "\n" +
              QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.") + "\n" +
              QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.");
#else
    // webkit dialogs for all platforms except android
#ifdef WIN32PORTABLE
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Portable");
    message = "<h2>" + QObject::tr("BASIC-256") + QObject::tr(" Portable") + "</h2>";
#else
    title = QObject::tr("About BASIC-256");
    message = "<h2>" + QObject::tr("BASIC-256") + "</h2>";
#endif	// WIN32PORTABLE

	message += QObject::tr("version ") + "<b>" + VERSION + "</b>" + QObject::tr(" - built with QT ") + "<b>" + QT_VERSION_STR + "</b>" +
            "<br>" + QObject::tr("Locale Name: ") + "<b>" + locale->name() + "</b> "+ QObject::tr("Decimal Point: ") + "<b>'" + (usefloatlocale?locale->decimalPoint():'.') + "'</b>" +
            "<p>" + QObject::tr("Copyright &copy; 2006-2020, The BASIC-256 Team") + "</p>" +
			"<p>" + QObject::tr("Please visit our web site at <a href=\"http://www.basic256.org\">http://www.basic256.org</a> for tutorials and documentation.") + "</p>" +
			"<p>" + QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.")  + "</p>" +
			"<p><i>" + QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.")  + "</i></p>";
#endif

    QMessageBox::about(this, title, message);
}



void MainWindow::configureGuiState() {
	//disable everything except what is needed to quit, stop and run a program.
	if (guiState==GUISTATERUN||guiState==GUISTATEAPP) {
		// common UI changes for both states
		filemenu_new_act->setVisible(false);
		filemenu_open_act->setVisible(false);
		filemenu_save_act->setVisible(false);
        filemenu_saveas_act->setVisible(false);
        filemenu_saveall_act->setVisible(false);
        filemenu_close_act->setVisible(false);
        filemenu_closeall_act->setVisible(false);
        filemenu_print_act->setVisible(false);
        editmenu->menuAction()->setVisible(false);
		undoact->setVisible(false);
		redoact->setVisible(false);
		cutact->setVisible(false);
		copyact->setVisible(false);
		pasteact->setVisible(false);
		debugact->setVisible(false);
		stepact->setVisible(false);
		bpact->setVisible(false);
		edit_whitespace_act->setVisible(false);
		clearbreakpointsact->setVisible(false);
        windowmenu->menuAction()->setVisible(false);

        findact->blockSignals(true);
		findagain->blockSignals(true);
		replaceact->blockSignals(true);
		helpthis->blockSignals(true);
		varwin_visible_act->setVisible(false);

        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            recentfiles_act[i]->setEnabled(false);
            recentfiles_act[i]->setVisible(false);
        }
        filemenu_recentfiles->menuAction()->setVisible(false);

        // application state additional changes
		if (guiState==GUISTATEAPP) {
            onlinehact->setVisible(false);
            editwin_visible_act->setChecked(false);
            editwin_visible_act->setVisible(false);
            main_toolbar_visible_act->setVisible(false);
            main_toolbar_visible_act->setChecked(false);
			runact->setVisible(false);
            stopact->setVisible(false);
            runmenu->menuAction()->setVisible(false);
		}
    }
}


void MainWindow::ifGuiStateRun() {
		// start run if app or run  state
		// called from main if code is loaded
		if (guiState==GUISTATEAPP || guiState==GUISTATERUN) {
			runact->activate(QAction::Trigger);
		}
}

void MainWindow::ifGuiStateClose(bool ok) {
	// optionally force close if application
	// from runtimecontroller when interperter stopps
    if (guiState==GUISTATEAPP){
        if(!ok){
            statusBar()->showMessage("Error.");
            QMessageBox::warning(this, tr("Error"), tr("Current program has terminated in an unusual way."));
        }
        close();
    }
}


void MainWindow::closeEvent(QCloseEvent *e) {
    // quit the application but ask if there are unsaved changes
    bool doquit = closeAllPrograms();
    if (doquit) {
        // save current screen posision, visibility and floating
        saveCustomizations();
        // actually quitting
        e->accept();
        QTimer::singleShot(0, qApp, SLOT(quit()));
        // close app as soon as the event loop is idle instead of using qApp->quit() to allow dispach of other events
        // This prevent app to not closing properly in rare situations like:
        // Interpreter emit() a blocking function in Controller (using QWaitCondition or BlockingQueuedConnection).
        // User request to close app while function is runnig in main loop. So, closeEvent is put in queue.
        // Function ends and return control to Interpreter. Interpreter request to run another code in main loop using emit().
        // Instead of running this code, the previous closeEvent() from queue is run.
        // Using qApp->quit() this will block forever Interpreter (never return), so, i->wait() will never return.
        // This is an old issue. It takes me a lot to manage it. (Florin)
    } else {
        // not quitting
        e->ignore();
    }
}

//Buttons section
void MainWindow::updateStatusBar(QString status) {
    statusBar()->showMessage(status);
}

void MainWindow::updateEditorButtons(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        const bool canEdit = !e->isReadOnly() && runState==RUNSTATESTOP;
        cutact->setEnabled(e->copyButton && canEdit);
        copyact->setEnabled(e->copyButton);
        undoact->setEnabled(e->undoButton && canEdit);
        redoact->setEnabled(e->redoButton && canEdit);
        pasteact->setEnabled(e->canPaste() && canEdit);
    }else{
        cutact->setEnabled(false);
        copyact->setEnabled(false);
        undoact->setEnabled(false);
        redoact->setEnabled(false);
        pasteact->setEnabled(false);
    }
}

void MainWindow::setRunState(int state) {
    // set the menus, menu options, and tool bar items to
    // correct state based on the stop/run/debug status
    // state see RUNSTATE* constants

    if(runState == state) return;
    runState = state;
    emit(setEditorRunState(state));

    const bool userCanInteractWithGUI = (state==RUNSTATESTOP && guiState==GUISTATENORMAL);
    const bool isEditorWindowActive = (editwin!=NULL);

    //enable/disable close buttons
    //change tab icon acording to runState
    QTabBar *tabBar = editwintabs->tabBar();
    int tabBarCount = tabBar->count();
    for (int i = 0; i < tabBarCount; i++){
        QWidget *w = tabBar->tabButton(i, QTabBar::RightSide);
        if(w) w->setEnabled(userCanInteractWithGUI);
        if(i==editwintabs->currentIndex()){
            if(state==RUNSTATEDEBUG || state==RUNSTATERUNDEBUG){
                editwintabs->setTabIcon(i, basicIcons->debugIcon);
            }else if(state==RUNSTATERUN){
                editwintabs->setTabIcon(i, basicIcons->runIcon);
            }else{
                editwintabs->setTabIcon(i, basicIcons->documentIcon);
            }
        }else{
            editwintabs->setTabIcon(i, basicIcons->documentIcon);
        }
    }



    // file menu
    filemenu_new_act->setEnabled(userCanInteractWithGUI);
    filemenu_open_act->setEnabled(userCanInteractWithGUI);
    filemenu_save_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    filemenu_saveas_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    filemenu_saveall_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    filemenu_close_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    filemenu_closeall_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    filemenu_print_act->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    recentfiles_empty_act->setEnabled(userCanInteractWithGUI);

    // edit menu
    updateEditorButtons();
    selectallact->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    findact->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    findagain->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    replaceact->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    beautifyact->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    prefact->setEnabled(userCanInteractWithGUI);

    // run menu
    runact->setEnabled(state==RUNSTATESTOP && isEditorWindowActive);
    debugact->setEnabled(userCanInteractWithGUI && isEditorWindowActive);
    stepact->setEnabled(state==RUNSTATEDEBUG || state==RUNSTATERUNDEBUG);
    bpact->setEnabled(state==RUNSTATEDEBUG);
    stopact->setEnabled(state!=RUNSTATESTOP && state!=RUNSTATESTOPING);
    clearbreakpointsact->setEnabled(state!=RUNSTATERUN && isEditorWindowActive);

    // Clear command for toolbars
    outwin->clearAct->setEnabled(userCanInteractWithGUI && !outwin->toPlainText().isEmpty());
    graphwin->clearAct->setEnabled(userCanInteractWithGUI);

    updateRecent();
}

//Check for an update
#ifndef ANDROID
void MainWindow::checkForUpdate(void){
    manager->get(request);
}

void MainWindow::sourceforgeReplyFinished(QNetworkReply* reply){
    QString url;
    QString filename;
    if(reply->error() == QNetworkReply::NoError) {
        QByteArray  strReply = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply);
        QJsonObject jsonObject = jsonResponse.object();
#if defined(WIN32PORTABLE) || defined(WIN32)
        filename = jsonObject["platform_releases"].toObject()["windows"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["windows"].toObject()["url"].toString();
#elif defined(LINUX)
        filename = jsonObject["platform_releases"].toObject()["linux"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["linux"].toObject()["url"].toString();
#elif defined(MACX)
        filename = jsonObject["platform_releases"].toObject()["mac"].toObject()["filename"].toString();
        url = jsonObject["platform_releases"].toObject()["mac"].toObject()["url"].toString();
#endif
        QRegExp rx("\\d+\\.\\d+\\.\\d+\\.\\d+");
        rx.indexIn(filename);
        QString siteversion = rx.cap(0);
        rx.indexIn(VERSION);
        QString thisversion = rx.cap(0);
        if(siteversion=="" || thisversion==""){
            //Unknown error
            if(!autoCheckForUpdate)QMessageBox::warning(this, tr("Check for an update"), tr("Unknown error."),QMessageBox::Ok, QMessageBox::Ok);
        }else if(siteversion>thisversion){
            //New version to download
            if(QMessageBox::information(this, tr("Check for an update"), tr("BASIC-256") + " " + siteversion + " " + tr("is now available - you have") + " " + thisversion + ". " + tr("Would you like to download the new version now?"),QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)==QMessageBox::Yes){
                QDesktopServices::openUrl(QUrl(url));
            }
        }else if(thisversion>siteversion){
            //Beta version
            if(!autoCheckForUpdate)QMessageBox::information(this, tr("Check for an update"), tr("You are currently using a development version."),QMessageBox::Ok, QMessageBox::Ok);
        }else{
            //No new version to download
            if(!autoCheckForUpdate)QMessageBox::information(this, tr("Check for an update"), tr("You are using the latest software version for your current OS."),QMessageBox::Ok, QMessageBox::Ok);
        }
        autoCheckForUpdate = false; //do automatically check for update only once

    } else {
        //Network error
        QMessageBox::warning(this, tr("Check for an update"), tr("We are unable to connect right now. Please check your network connection and try again."),QMessageBox::Ok, QMessageBox::Ok);
    }
    reply->deleteLater();
}
#endif




void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    if (event->mimeData()->hasFormat("text/uri-list") && guiState==GUISTATENORMAL){
        event->acceptProposedAction();
    }else{
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event){
        if(event->mimeData()->hasFormat("text/uri-list")){
        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.isEmpty())
            return;
        QString fileName = urls.first().toLocalFile();
        if (fileName.isEmpty())
            return;
        if(runState != RUNSTATESTOP) rc->stopRun();
        loadFile(fileName);
    }
    return;
}


void MainWindow::updateRecent() {
    //update recent list on file menu
    if (guiState==GUISTATENORMAL) {
        int counter=0;
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        QString path = QDir::currentPath() + "/";
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            QString fn = settings.value(QString::number(i), "").toString().trimmed();
            if(!fn.isEmpty()){
                recentfiles_act[counter]->setData(fn);
                recentfiles_act[counter]->setStatusTip(fn);
                if (QString::compare(path, fn.left(path.length()))==0) {
                    fn = fn.right(fn.length()-path.length());
                }
                recentfiles_act[counter]->setEnabled(runState == RUNSTATESTOP);
                recentfiles_act[counter]->setVisible(true);
                recentfiles_act[counter]->setText((counter>=9?QString::number(int((counter+1)/10)):QString("")) + "&" + QString::number((counter+1)%10) + " - " + fn);
                counter++; //next slot - skip empty values deleted manually by user
            }
        }
        settings.endGroup();
        //disable unused slots
        for (int i=counter; i<SETTINGSGROUPHISTN; i++) {
            recentfiles_act[i]->setEnabled(false);
            recentfiles_act[i]->setVisible(false);
        }
        //if we have no history to show
        if(counter==0){
            recentfiles_empty_act->setEnabled(false);
            filemenu_recentfiles->setEnabled(false);
        }else{
            recentfiles_empty_act->setEnabled(runState == RUNSTATESTOP);
            filemenu_recentfiles->setEnabled(runState == RUNSTATESTOP);
        }
    }
}

void MainWindow::openRecent(){
    QAction *action = qobject_cast<QAction *>(sender());
    if (action){
        QString f = action->data().toString().trimmed();
        if(!f.isEmpty()){
            loadFile(f);
        }
    }
}

void MainWindow::emptyRecent(){
    SETTINGS;
    settings.beginGroup(SETTINGSGROUPHIST);
        for (int i=0; i<SETTINGSGROUPHISTN; i++) {
            settings.setValue(QString::number(i), "");
        }
    settings.endGroup();
    updateRecent();
}

void MainWindow::addFileToRecentList(QString fn) {
    // keep list of recently open or saved files
    // put file name at position 0 on list
    fn=fn.trimmed();
    if(!fn.isEmpty()){
        SETTINGS;
        settings.beginGroup(SETTINGSGROUPHIST);
        // if program is at top then do nothing
        if (settings.value(QString::number(0), "").toString() != fn) {
            // find end of scootdown
            int e;
            for(e=1; e<SETTINGSGROUPHISTN && settings.value(QString::number(e), "").toString() != fn; e++) {}
            // scoot entries down
            for (int i=(e<SETTINGSGROUPHISTN?e:SETTINGSGROUPHISTN-1); i>=1; i--) {
                settings.setValue(QString::number(i), settings.value(QString::number(i-1), ""));
            }
            settings.setValue(QString::number(0), fn);
        }
        settings.endGroup();
        updateRecent();
    }
}


void MainWindow::dialogFontSelect() {
    bool ok;
    editorFont = QFontDialog::getFont(&ok, editorFont, this, QString(), QFontDialog::MonospacedFonts);
    if (ok) {
        mymutex->lock();
        if(guiState!=GUISTATEAPP){
            for(int i=0; i<editwintabs->count(); i++){
                ((BasicEdit*)editwintabs->widget(i))->setFont(editorFont);
            }
        }
        outwin->setFont(editorFont);
        waitCond->wakeAll();
        mymutex->unlock();
    }
}


void MainWindow::activeEditorPrint(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->slotPrint();
    }
}
void MainWindow::activeEditorSaveProgram(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->removePath(e->filename);
        e->saveProgram();
        if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->addPath(e->filename);
    }
}
void MainWindow::activeEditorSaveAsProgram(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->removePath(e->filename);
        e->saveAsProgram();
        if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->addPath(e->filename);
    }
}
void MainWindow::activeEditorUndo(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->undo();
    }
}
void MainWindow::activeEditorRedo(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->redo();
    }
}
void MainWindow::activeEditorCut(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->cut();
    }
}
void MainWindow::activeEditorCopy(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->copy();
    }
}
void MainWindow::activeEditorPaste(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->paste();
    }
}
void MainWindow::activeEditorSelectAll(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->selectAll();
    }
}
void MainWindow::activeEditorBeautifyProgram(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->beautifyProgram();
    }
}
void MainWindow::activeEditorClearBreakPoints() {
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->clearBreakPoints();
    }
}
void MainWindow::updateWindowTitle(BasicEdit* editor) {
    if(editor){
        QString tabTitle(editor->windowtitle);
        int i=editwintabs->indexOf((QWidget *)editor);
        if(i>=0){
            editwintabs->setTabText(i,tabTitle);
            if(i==editwintabs->currentIndex()) setWindowTitle(tabTitle + QObject::tr(" - BASIC-256"));
        }
    }
}
void MainWindow::currentEditorTabChanged(int tab){
    BasicEdit *e = (BasicEdit*)editwintabs->widget(tab);
    if(e){
        updateWindowTitle(e);
        e->cursorMove();
        e->highlightCurrentLine();
        e->setFocus();
        QTabBar *tabBar = editwintabs->tabBar();
        int tabBarCount = tabBar->count();
        for (int i = 0; i < tabBarCount; i++){
            QWidget *w = tabBar->tabButton(i, QTabBar::RightSide);
            if(w){
                if (i != tab)
                {
                    w->hide();
                }else{
                    w->show();
                }
            }
        }
    }
    updateEditorButtons();
    editwin = e;

    //Update menu if there is an opened window editor or not
    const bool val = (editwin!=NULL && runState==RUNSTATESTOP);
    filemenu_save_act->setEnabled(val);
    filemenu_saveas_act->setEnabled(val);
    filemenu_saveall_act->setEnabled(val);
    filemenu_close_act->setEnabled(val);
    filemenu_closeall_act->setEnabled(val);
    filemenu_print_act->setEnabled(val);
    // edit menu
    selectallact->setEnabled(val);
    findact->setEnabled(val);
    findagain->setEnabled(val);
    replaceact->setEnabled(val);
    beautifyact->setEnabled(val);
    // run menu
    runact->setEnabled(val);
    debugact->setEnabled(val);
    clearbreakpointsact->setEnabled(editwin!=NULL && runState!=RUNSTATERUN);
}

void MainWindow::closeEditorTab(int tab){
    if(runState!=RUNSTATESTOP) return;
    BasicEdit *e = (BasicEdit*)editwintabs->widget(tab);
    if(e){
        bool doclose = true;
        if (e->document()->isModified()) {
            doclose = ( QMessageBox::Yes == QMessageBox::warning(this, tr("Program modifications have not been saved."),
                tr("Do you want to discard your changes?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No));
        }
        if (doclose) {
            if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->removePath(e->filename);
            e->deleteLater();
        }
    }
}

void MainWindow::setCurrentEditorTab(BasicEdit* e){
    if(e){
        editwintabs->setCurrentWidget((QWidget *)e);
    }
}

void MainWindow::activeEditorCloseTab(){
    closeEditorTab(editwintabs->currentIndex());
}

BasicEdit* MainWindow::newEditor(QString title){
    BasicEdit* editor = new BasicEdit(title);

    if(guiState!=GUISTATEAPP){
        editor->setFont(editorFont);
        editor->slotWhitespace(edit_whitespace_act->isChecked());
        editsyntax = new EditSyntaxHighlighter(editor->document());
        // connect the signals
        QObject::connect(edit_whitespace_act, SIGNAL(toggled(bool)), editor, SLOT(slotWhitespace(bool)));
        QObject::connect(editor, SIGNAL(changeStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
        QObject::connect(editor, SIGNAL(updateWindowTitle(BasicEdit*)), this, SLOT(updateWindowTitle(BasicEdit*)));
        QObject::connect(this, SIGNAL(setEditorRunState(int)), editor, SLOT(setEditorRunState(int)));
        if(guiState!=GUISTATERUN){
            QObject::connect(editor, SIGNAL(updateEditorButtons()), this, SLOT(updateEditorButtons()));
            QObject::connect(editor, SIGNAL(addFileToRecentList(QString)), this, SLOT(addFileToRecentList(QString)));
            QObject::connect(editor, SIGNAL(setCurrentEditorTab(BasicEdit*)), this, SLOT(setCurrentEditorTab(BasicEdit*)));
            QObject::connect(this, SIGNAL(saveAllStep(int)), editor, SLOT(saveAllStep(int)));
            QObject::connect(fileSystemWatcher, SIGNAL(fileChanged(QString)), editor, SLOT(fileChangedOnDiskSlot(QString)) );
        }
    }
    return editor;
}

void MainWindow::newProgram(){
    BasicEdit *neweditor = newEditor(QObject::tr("Untitled") + QString(" ") + QString::number(untitledNumber++));
    editwin = neweditor;
    //add tab and make it active
    int i = editwintabs->addTab(neweditor, neweditor->title);
    editwintabs->setTabIcon(i, basicIcons->documentIcon);
    editwintabs->setCurrentIndex(i);
    neweditor->setFocus();
}

void MainWindow::loadProgram() {
    QString s = QFileDialog::getOpenFileName(this, QObject::tr("Open a file"), ".", QObject::tr("BASIC-256 file ") + "(*.kbs);;" + QObject::tr("Any File ") + "(*.*)");
    loadFile(s);
}

bool MainWindow::loadFile(QString s) {
    s = s.trimmed();
    if (s != NULL) {
        bool doload = true;
            if (QFile::exists(s)) {
                QFile f(s);
                if (f.open(QIODevice::ReadOnly)) {
                    QFileInfo fi(f);
                    QString filename = fi.absoluteFilePath();

                    //check if file is already open
                    for(int i=0; i<editwintabs->count(); i++){
                        BasicEdit* e = (BasicEdit*)editwintabs->widget(i);
                        if(e && e->filename==filename){
                            f.close();
                            editwintabs->setCurrentIndex(i);
                            return true;
                        }
                    }

                    QMimeDatabase db;
                    QMimeType mime = db.mimeTypeForFile(fi);
                    // Get user confirmation for non-text files
                    //Remember that empty ".kbs" files are detected as non-text files
                    if (!(mime.inherits("text/plain") && !(fi.fileName().endsWith(".kbs",Qt::CaseInsensitive) && fi.size()==0))) {
                        doload = ( QMessageBox::Yes == QMessageBox::warning(this, QObject::tr("Load File"),
                            QObject::tr("It does not seem to be a text file.")+ "\n" + QObject::tr("Load it anyway?"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No));
                    }else if (!fi.fileName().endsWith(".kbs",Qt::CaseInsensitive)) {
                        doload = ( QMessageBox::Yes == QMessageBox::warning(this, QObject::tr("Load File"),
                            QObject::tr("You're about to load a file that does not end with the .kbs extension.")+ "\n" + QObject::tr("Load it anyway?"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No));
                    }
                    if (doload) {
                        //replace empty document created by default (if exists)
                        bool replaceEmptyDoc = false;
                        BasicEdit *neweditor;
                        if(untitledNumber==2){
                            BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
                            if(e){
                                if(e->filename.isEmpty() && !e->document()->isModified()){
                                    neweditor=e;
                                    replaceEmptyDoc=true;
                                }
                            }
                        }
                        if(!replaceEmptyDoc) neweditor = newEditor(fi.fileName());
                        editwin = neweditor;
                        neweditor->filename = filename;
                        neweditor->path = fi.absolutePath();
                        neweditor->title=fi.fileName();

                        updateStatusBar(QObject::tr("Loading file..."));
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        QByteArray ba = f.readAll();
                        f.close();
                        neweditor->setPlainText(QString::fromUtf8(ba.data()));
                        neweditor->document()->setModified(false);
                        setWindowTitle(fi.fileName());
                        addFileToRecentList(s);
                        QApplication::restoreOverrideCursor();
                        updateStatusBar(QObject::tr("Ready."));
                        if(fileSystemWatcher) fileSystemWatcher->addPath(filename);

                        //add tab and make it active
                        if(!replaceEmptyDoc){
                            int i = editwintabs->addTab(neweditor, neweditor->title);
                            editwintabs->setTabIcon(i, basicIcons->documentIcon);
                            editwintabs->setCurrentIndex(i);
                        }else{
                            neweditor->updateTitle();
                        }
                        return true;
                    }
                    f.close();
                } else {
                    QMessageBox::warning(this, QObject::tr("Load File"),
                        QObject::tr("Unable to open program file")+" \""+s+"\".\n"+QObject::tr("File permissions problem or file open by another process."),
                        QMessageBox::Ok, QMessageBox::Ok);
                }
            } else {
                QMessageBox::warning(this, QObject::tr("Load File"),
                    QObject::tr("Program file does not exist.")+" \""+s+QObject::tr("\"."),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
        }
return false;
}

void MainWindow::updateWindowMenu(){
    windowmenu->addAction(filemenu_close_act);
    windowmenu->addAction(filemenu_closeall_act);
    windowmenu->addSeparator();
    for(int i=0; i<editwintabs->count(); i++){
        BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
        if(e){
            QAction *a = e->action;
            if(a){
                a->setChecked(i==editwintabs->currentIndex());
                windowmenu->addAction(a);
            }
        }
    }
}

void MainWindow::saveAll(){
    // Step 1 - save changes
    emit(saveAllStep(1));
    // Step 2 - save unsaved files (need user interaction)
    emit(saveAllStep(2));
}

bool MainWindow::closeAllPrograms(){
    QString list;
    bool doit = true;
    int count = 0;
    //count for unsaved files
    for(int i=0; i<editwintabs->count(); i++){
        BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
        if(e){
            if(e->document()->isModified()){
                list.append(e->title).append("\n");
                count++;
            }
        }
    }
    //if there are unsaved files
    if(count!=0){
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(QObject::tr("Save changes?"));
        if(count==1){
            msgBox.setText(QObject::tr("The following file have unsaved changes:"));
        }else{
            msgBox.setText(QObject::tr("The following files have unsaved changes:"));
        }
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setInformativeText(list);
        msgBox.setStandardButtons( (count==1?QMessageBox::Save:QMessageBox::SaveAll) | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int doclose = msgBox.exec();
        if(doclose==QMessageBox::SaveAll || doclose==QMessageBox::Save){
            saveAll();
        }else if(doclose==QMessageBox::Cancel){
            return false;
        }

        //double check for unsaved files - user press [Cancel] at saving time
        count = 0;
        for(int i=0; i<editwintabs->count(); i++){
            BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
            if(e){
                if(e->document()->isModified()){
                    count++;
                }
            }
        }
        if(count>0){
            doit = ( QMessageBox::Yes == QMessageBox::warning(this, QObject::tr("Unsaved files"),
                QObject::tr("There are unsaved files.")+ "\n" + QObject::tr("Do you really want to discard your changes?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No));
        }




    }
    if(doit){
        rc->stopRun();
        for(int i=editwintabs->count()-1; i>=0; i--){
            BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
            e->deleteLater();
        }
    }
    return doit;
}


void MainWindow::updateBreakPointsAction(){
    bool val = false;
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e) val = e->isBreakPoint();
    clearbreakpointsact->setEnabled(val);
}

void MainWindow::zoomGroupActionEvent(QAction* a){
    graphwin->slotSetZoom(a->data().toDouble());
}
