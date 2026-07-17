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
#include <QRegularExpression>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtGui/QFontDatabase>
#include <QtGui/QFontInfo>
#include <QtGui/QFontMetrics>
#include <QtGui/QShortcut>
#ifdef Q_OS_WASM
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <utility>
#endif

#include <QScreen>
#include <QTimer>

#include "MainWindow.h"
#include "Settings.h"
#include "Version.h"
#include "BasicDock.h"
#include "BasicIcons.h"
#include "BasicKeyboard.h"
#include "EditSyntaxHighlighter.h"

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


MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags f, QString localestring, int guistate, bool fullscreen)
    :	QMainWindow(parent, f) {

    localecode = localestring;
	locale = new QLocale(localecode);
    guiState = guistate;
    guiFullScreen = fullscreen;
    mainwin = this;
    setAcceptDrops(true);
    untitledNumber = 1;
    runState = RUNSTATESTOP;
    quitConfirmed = false;
    editwin=NULL;
    basicIcons = new BasicIcons();
    basicKeyboard = new BasicKeyboard();


    // create the global mymutexes and waits
    mymutex = new QMutex();
    mydebugmutex = new QMutex();
    waitCond = new QWaitCondition();
    waitDebugCond = new QWaitCondition();

    setWindowIcon(basicIcons->basic256Icon);

#ifndef Q_OS_WASM
    // Checks for a newer *desktop* download -- meaningless in a browser
    // (there's nothing to "download and install" over the running page),
    // and confirmed via a real browser test to fail anyway: sourceforge.net
    // doesn't send CORS headers allowing this cross-origin fetch, so it was
    // just a silent, guaranteed-to-fail network call on every WASM startup.
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
    // Windows' native style (unlike Linux) draws the tab close button as a
    // plain glyph with no background, making the active tab hard to spot
    // among several. Force a visible background so it stands out on every
    // platform/style. Once QSS touches ::close-button at all, Qt stops
    // drawing the style's built-in glyph on top of it -- supply one
    // explicitly via Qt's own bundled commonstyle icon, or the button
    // renders as a plain colored box with no X in it.
    editwintabs->tabBar()->setStyleSheet(
        "QTabBar::close-button {"
        "    background-color: #e81123;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "    image: url(:/qt-project.org/styles/commonstyle/images/standardbutton-closetab-16.png);"
        "}"
        "QTabBar::close-button:hover {"
        "    background-color: #f1707a;"
        "    image: url(:/qt-project.org/styles/commonstyle/images/standardbutton-closetab-hover-16.png);"
        "}"
    );



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
    // Open Example: on WASM the programs are read from Qt resources; on desktop
    // they live on disk relative to the executable. Either way the item earns
    // its place -- inside an AppImage the bundled Examples/ sits at an ephemeral
    // squashfs mount path the user can't navigate to by hand, so a menu entry
    // that resolves it for them is the only practical way to reach it.
    filemenu_openexample_act = filemenu->addAction(basicIcons->openIcon, QObject::tr("Open &Example..."));

    // Recent files menu
    filemenu_recentfiles = filemenu->addMenu(QObject::tr("Open &Recent"));
    for(int i=0;i<SETTINGSGROUPHISTN;i++){
        recentfiles_act[i] = filemenu_recentfiles->addAction(basicIcons->openIcon, QObject::tr(""));
        if(i<10)
            recentfiles_act[i]->setShortcut(QKeySequence(Qt::Key(Qt::Key_0 + ((i+1)%SETTINGSGROUPHISTN)) | Qt::CTRL));
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
    edit_wrap_act = viewmenu->addAction(QObject::tr("W&rap Long Lines"));
    edit_wrap_act->setCheckable(true);
    edit_wrap_act->setChecked(SETTINGSEDITWRAPDEFAULT);
    
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
    // F1 (HelpContents) is the context-sensitive lookup on the keyword under
    // the cursor (helpthis); the general docs homepage (onlinehact) moves to
    // Shift+F1 (WhatsThis). Users expect F1 to open help for the keyword.
    onlinehact->setShortcuts(QKeySequence::keyBindings(QKeySequence::WhatsThis));
    helpthis = new QAction (this);
    helpthis->setShortcuts(QKeySequence::keyBindings(QKeySequence::HelpContents));
    addAction (helpthis);
    helpmenu->addSeparator();
    checkupdate = helpmenu->addAction(QObject::tr("&Check for update..."));
    helpmenu->addSeparator();
    QAction *aboutact = helpmenu->addAction(basicIcons->infoIcon, QObject::tr("&About BASIC-256..."));

    // Add actions to main window toolbar
    main_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
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

    //resize(800, 800);  // your preferred default
	loadCustomizations();
    configureGuiState();

    //Display windows and toolbars acording their final settings
    // Single-output modes have their layout owned entirely by configureGuiState();
    // restoring dock visibility from saved settings would undo that work.
    // --silent never shows anything, so it is excluded here too.
    if (guiState != GUISTATEGRAPH && guiState != GUISTATETEXT && guiState != GUISTATESILENT) {
        editwintabs->setVisible(editwin_visible_act->isChecked());
        graphwin_dock->setVisible(graphwin_visible_act->isChecked());
        outwin_dock->setVisible(outwin_visible_act->isChecked());
        varwin_dock->setVisible(varwin_visible_act->isChecked());
    }
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
    QObject::connect(filemenu_closeall_act, SIGNAL(triggered()), this, SLOT(closeAllProgramsSlot()));
    QObject::connect(filemenu_openexample_act, SIGNAL(triggered()), this, SLOT(openExample()));
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

#ifndef Q_OS_WASM
    QObject::connect(checkupdate, SIGNAL(triggered()), this, SLOT(checkForUpdate()));
#endif
QObject::connect(helpthis, SIGNAL(triggered()), rc, SLOT(showOnlineContextDocumentation()));


#ifndef Q_OS_WASM
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
    QByteArray geo = settings.value(
        SETTINGSMAINGEOMETRY + QString::number(guiState)).toByteArray();
    if (!geo.isEmpty() && geo.size() > 4) {
        restoreGeometry(geo);   // silently no-ops if data is wrong format
    } else {
        // First time this mode has ever run: without an explicit default,
        // QMainWindow sizes itself from its Expanding-policy dock widgets'
        // sizeHint (which tends to end up close to the whole screen) and
        // always lands at Qt's default top-left placement. Pick a sane,
        // centered default instead.
        QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
        QSize defaultSize(qMin(1024, avail.width()), qMin(768, avail.height()));
        resize(defaultSize);
        move(avail.center() - QPoint(defaultSize.width() / 2, defaultSize.height() / 2));
    }


    if(restoreWindows){
        restoreGeometry(settings.value(SETTINGSMAINGEOMETRY + QString::number(guiState)).toByteArray());
        QByteArray state = settings.value(SETTINGSMAINSTATE + QString::number(guiState)).toByteArray();
        // Skipping restoreState() here also prevents a previously-floating dock
        // from being recreated as its own top-level window in --silent mode.
        if (guiState != GUISTATEGRAPH && guiState != GUISTATETEXT && guiState != GUISTATESILENT) {
            restoreState(state);
        }
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

    // edit whitespace and wrap
    v = settings.value(SETTINGSEDITWHITESPACE + QString::number(guiState), SETTINGSEDITWHITESPACEDEFAULT).toBool();
    edit_whitespace_act->setChecked(v);
    v = settings.value(SETTINGSEDITWRAP + QString::number(guiState), SETTINGSEDITWRAPDEFAULT).toBool();
    edit_wrap_act->setChecked(v);

    // graph toolbar and grid
    v = settings.value(SETTINGSGRAPHGRIDLINES + QString::number(guiState), SETTINGSGRAPHGRIDLINESDEFAUT).toBool();
    graph_grid_visible_act->setChecked(v);
    v = settings.value(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), SETTINGSGRAPHTOOLBARVISIBLEDEFAULT).toBool();
    graphwin_toolbar_visible_act->setChecked(v);

    // out toolbar
    v = settings.value(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), SETTINGSOUTTOOLBARVISIBLEDEFAULT).toBool();
    outwin_toolbar_visible_act->setChecked(v);

    // set initial font
    //
    // The menus follow the system text size (Main.cpp pushes Windows'
    // lfMessageFont into QApplication::setFont()); the editor and Text Output did
    // not, because they loaded a hardcoded "DejaVu Sans Mono,11". So raising the
    // system text size grew the menus and left the code at 11pt.
    //
    // Now: unless the user has explicitly picked a font, derive it from the system
    // on every launch, so it keeps tracking the system size rather than freezing
    // at whatever it happened to be the first time. Persisting an auto-derived
    // size would defeat the whole point -- it would stop following the moment the
    // system setting changed again.
    QString initialFontString = settings.value(SETTINGSFONT + QString::number(guiState), QString()).toString();

    if (settings.contains(SETTINGSFONTUSERSET + QString::number(guiState))) {
        fontUserSet = settings.value(SETTINGSFONTUSERSET + QString::number(guiState)).toBool();
    } else {
        // Pre-existing profile with no flag to read, so infer one -- once. Every
        // run wrote the font back out on close, so somebody who never opened the
        // font dialog still has the old hardcoded default stored: treat exactly
        // that as "never chosen". Anything else was a deliberate choice, and is
        // kept. saveCustomizations() then persists the flag, so this inference
        // never runs again -- it must not, because by then the stored font is the
        // system-derived one and would look like a deliberate choice.
        QFont legacy;
        legacy.fromString(SETTINGSFONTDEFAULT);
        QFont stored;
        if (!initialFontString.isEmpty()) stored.fromString(initialFontString);
        fontUserSet = !initialFontString.isEmpty() &&
                      !(stored.family() == legacy.family() && stored.pointSize() == legacy.pointSize());
    }

    if (fontUserSet && !initialFontString.isEmpty()) {
        editorFont.fromString(initialFontString);
        // A pinned font can only have come from Options > Font, which offers
        // monospaced faces only (QFontDialog::MonospacedFonts). So a pinned
        // font that is not fixed-pitch was never actually chosen -- it is a
        // leftover from an old build that saved the proportional app font as
        // the editor font (e.g. "MS Shell Dlg 2,8.25"), which the DejaVu-only
        // legacy-default check above did not catch. Un-pin it and re-derive,
        // so the editor goes back to following the system text size. The
        // cleared flag is persisted by saveCustomizations(), so this runs once.
        if (!QFontInfo(editorFont).fixedPitch()) {
            fontUserSet = false;
            editorFont = defaultEditorFont();
        }
    } else {
        editorFont = defaultEditorFont();
    }
    outwin->setFont(editorFont);

    autoCheckForUpdate = (guiState==GUISTATENORMAL&&settings.value(SETTINGSCHECKFORUPDATE, SETTINGSCHECKFORUPDATEDEFAULT).toBool());
}


void MainWindow::saveCustomizations() {
    // save user customizations on close

    SETTINGS;
    // -f is a one-off, per-invocation request, not a saved preference: if we
    // persisted the fullscreen geometry here, the next plain (non -f) run of
    // this same mode would restore it via loadCustomizations() and appear
    // fullscreen too, since geometry is keyed only by guiState, with no
    // distinction for whether -f was passed that time. Only guard the modes
    // where -f actually has an effect (matches configureGuiState()'s check),
    // so a stray -f with no mode flag doesn't stop normal IDE geometry saves.
    bool skipGeometrySave = guiFullScreen &&
        (guiState == GUISTATERUN || guiState == GUISTATEAPP ||
         guiState == GUISTATETEXT || guiState == GUISTATEGRAPH);
    if (!skipGeometrySave) {
        settings.setValue(SETTINGSMAINGEOMETRY + QString::number(guiState), saveGeometry());
        settings.setValue(SETTINGSMAINSTATE + QString::number(guiState), saveState());
    }

    // main
	settings.setValue(SETTINGSTOOLBARVISIBLE + QString::number(guiState), main_toolbar->isVisible());

    // edit
	settings.setValue(SETTINGSEDITVISIBLE + QString::number(guiState), editwin_visible_act->isChecked());
	settings.setValue(SETTINGSEDITWHITESPACE + QString::number(guiState), edit_whitespace_act->isChecked());
	settings.setValue(SETTINGSEDITWRAP + QString::number(guiState), edit_wrap_act->isChecked());

    // graph
	settings.setValue(SETTINGSGRAPHVISIBLE + QString::number(guiState), graphwin_visible_act->isChecked());
	settings.setValue(SETTINGSGRAPHTOOLBARVISIBLE + QString::number(guiState), graphwin_widget->isVisibleToolBar());
	settings.setValue(SETTINGSGRAPHGRIDLINES + QString::number(guiState), graphwin->isVisibleGridLines());

    // out
	settings.setValue(SETTINGSOUTVISIBLE + QString::number(guiState), outwin_visible_act->isChecked());
	settings.setValue(SETTINGSOUTTOOLBARVISIBLE + QString::number(guiState), outwin_widget->isVisibleToolBar());

    // var
	settings.setValue(SETTINGSVARVISIBLE + QString::number(guiState), varwin_visible_act->isChecked());

    // font (the flag too: while it is false the stored font is ignored on load
    // and re-derived from the system, so the editor keeps following the system
    // text size instead of freezing at whatever it was the first time)
    settings.setValue(SETTINGSFONT + QString::number(guiState), editorFont.toString());
    settings.setValue(SETTINGSFONTUSERSET + QString::number(guiState), fontUserSet);

    // zoom
    settings.setValue(SETTINGSZOOM, QString::number(graphwin->getZoom()));
}

void MainWindow::resizeToFitGraph(int canvasW, int canvasH) {
    // -f wins: re-assert fullscreen instead of shrinking to the canvas size.
    // configureGuiState()'s setGeometry(avail) call (at construction time, before
    // show()) doesn't reliably stick for GUISTATEGRAPH: setCentralWidget() there
    // posts a deferred layout request that resizes the window down to the small
    // graph widget's sizeHint once the event loop catches up, which happens after
    // that setGeometry() call and undoes it. By the time resizeToFitGraph() runs
    // (script start, window already shown and settled) that layout request has
    // long since fired, so re-applying fullscreen here is what actually sticks -
    // equivalent to the user manually maximizing the window afterward.
    if (guiFullScreen) {
        QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
        setGeometry(avail);
        return;
    }

    // Compute the chrome height: everything between the OS client area and the
    // scroll-area viewport that holds graphwin.
    // = menu bar + status bar  (main window level)
    // + graphwin_widget toolbar (BasicWidget level)
    // graph_scroll sits inside graphwin_widget; the height difference is the toolbar.
    int chromeH = 0;
    if (menuBar() && menuBar()->isVisible())
        chromeH += menuBar()->height();
    if (statusBar() && statusBar()->isVisible())
        chromeH += statusBar()->height();
    chromeH += graphwin_widget->height() - graph_scroll->height();
    resize(canvasW, canvasH + chromeH);

    // resize() doesn't reposition, so without this the window is left wherever
    // it happened to be before shrinking down to the canvas size.
    QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    move(avail.center() - QPoint(width() / 2, height() / 2));
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


#ifdef WIN32PORTABLE
    title = QObject::tr("About BASIC-256") +  QObject::tr(" Portable");
    message = "<h2>" + QObject::tr("BASIC-256") + QObject::tr(" Portable") + "</h2>";
#else
    title = QObject::tr("About BASIC-256");
    message = "<h2>" + QObject::tr("BASIC-256") + "</h2>";
#endif	// WIN32PORTABLE

	message += QObject::tr("version ") + "<b>" + VERSION + "</b>" + QObject::tr(" - built with QT ") + "<b>" + QT_VERSION_STR + "</b>" +
            "<br>" + QObject::tr("Locale Name: ") + "<b>" + locale->name() + "</b> "+ QObject::tr("Decimal Point: ") + "<b>'" + (usefloatlocale?locale->decimalPoint():QChar('.')) + "'</b>" +
            "<p>" + QObject::tr("Copyright &copy; 2006-2026, The BASIC-256 Team") + "</p>" +
			"<p>" + QObject::tr("Please visit our web site at <a href=\"https://uglymike17.github.io/Basic256-Docs/\">https://uglymike17.github.io/Basic256-Docs/</a> for documentation.") + "</p>" +
			"<p>" + QObject::tr("Please see the CONTRIBUTORS file for a list of developers and translators for this project.")  + "</p>" +
			"<p><i>" + QObject::tr("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.")  + "</i></p>";

    // Async (RULE 2): the static QMessageBox::about()'s exec() never
    // returns on the WASM main thread without Asyncify -- build the same
    // dialog manually and show it non-modally instead.
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle(title);
    msgBox->setText(message);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setStandardButtons(QMessageBox::Ok);
    // open() forces window-modal, which unlike exec()'s old application-modal
    // default doesn't disable the main window's native title bar on Windows --
    // closing the app while About is up would re-enter closeEvent().
    msgBox->setWindowModality(Qt::ApplicationModal);
    msgBox->show();
}



void MainWindow::configureGuiState() {

    // ── TIER 1: common to every non-normal mode ────────────────────────────
    bool isRestricted = (guiState == GUISTATERUN  ||
                         guiState == GUISTATEAPP  ||
                         guiState == GUISTATEGRAPH||
                         guiState == GUISTATETEXT ||
                         guiState == GUISTATESILENT);

    if (isRestricted) {
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
        for (int i = 0; i < SETTINGSGROUPHISTN; i++) {
            recentfiles_act[i]->setEnabled(false);
            recentfiles_act[i]->setVisible(false);
        }
        filemenu_recentfiles->menuAction()->setVisible(false);
    }

    // ── TIER 2: extra cleanup for app/graph/text modes (no IDE at all) ─────
    bool isKioskMode = (guiState == GUISTATEAPP  ||
                        guiState == GUISTATEGRAPH ||
                        guiState == GUISTATETEXT ||
                        guiState == GUISTATESILENT);

    if (isKioskMode) {
        onlinehact->setVisible(false);
        editwin_visible_act->setChecked(false);
        editwin_visible_act->setVisible(false);
        main_toolbar_visible_act->setVisible(false);
        main_toolbar_visible_act->setChecked(false);
        runact->setVisible(false);
        stopact->setVisible(false);
        runmenu->menuAction()->setVisible(false);
    }

    // ── TIER 3: single-output-window modes ────────────────────────────────
    if (guiState == GUISTATEGRAPH) {
        outwin_dock->hide();
        outwin_visible_act->setVisible(false);
        varwin_dock->hide();
        viewmenu->menuAction()->setVisible(false);

        QWidget *gw = graphwin_dock->widget();   // ← capture before detaching
        removeDockWidget(graphwin_dock);
        graphwin_dock->setWidget(nullptr);
        QWidget *old = takeCentralWidget();
        old->hide();
        setCentralWidget(gw);                    // ← use local, no naming ambiguity
        gw->show();
        graphwin_dock->hide();

    } else if (guiState == GUISTATETEXT) {
        graphwin_dock->hide();
        graphwin_visible_act->setVisible(false);
        varwin_dock->hide();
        viewmenu->menuAction()->setVisible(false);

        QWidget *tw = outwin_dock->widget();     // ← capture before detaching
        removeDockWidget(outwin_dock);
        outwin_dock->setWidget(nullptr);
        QWidget *old = takeCentralWidget();
        old->hide();
        setCentralWidget(tw);                    // ← use local, no naming ambiguity
        tw->show();
        outwin_dock->hide();
    }

    // --full: apply after restoreGeometry() so it always wins over saved state.
    // Only meaningful alongside -r / -a / -g / -t; silently ignored otherwise.
    if (guiFullScreen) {
        QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
        if (guiState == GUISTATERUN || guiState == GUISTATEAPP ||
            guiState == GUISTATETEXT || guiState == GUISTATEGRAPH) {
            setGeometry(avail);
        }
    }
}

void MainWindow::ifGuiStateRun() {
	// start run if app or run  state
	// called from main if code is loaded
    if (guiState == GUISTATEAPP  ||
        guiState == GUISTATERUN  ||
        guiState == GUISTATEGRAPH||
        guiState == GUISTATETEXT ||
        guiState == GUISTATESILENT) {
        runact->activate(QAction::Trigger);
    }
}

void MainWindow::ifGuiStateClose(bool ok) {
	// optionally force close if application
	// from runtimecontroller when interperter stopps
    if (guiState==GUISTATESILENT){
        // No window was ever shown, so there is no close/GUI-close-event path
        // and no modal dialogs. Defer the exit like the GUISTATEAPP close path
        // below does (see the closeEvent() comment): quitting immediately from
        // inside this queued slot can race with any events the interpreter has
        // already posted.
        QTimer::singleShot(0, this, [ok]() { qApp->exit(ok ? 0 : 1); });
        return;
    }
    if (guiState==GUISTATEAPP){
        if(!ok){
            statusBar()->showMessage("Error.");
            QMessageBox::warning(this, tr("Error"), tr("Current program has terminated in an unusual way."));
        }
        close();
    }
}


void MainWindow::closeEvent(QCloseEvent *e) {
	if (quitConfirmed) {
		// We already ran the unsaved-changes flow once and decided to quit.
		// This call is Qt re-entering closeEvent() from qApp->quit()'s own
		// QEvent::Quit handling, which calls closeAllWindows() to double-check
		// every top-level widget is willing to close before actually exiting.
		// Accept immediately instead of re-running closeAllPrograms() (which
		// would just ignore() again and make Qt think the close failed,
		// scheduling yet another quit() -- an infinite closeEvent loop).
		e->accept();
		return;
	}
	if(runState == RUNSTATERUN) {
		// cause interpreter to do a controlled stop and wait for stop to finish
		rc->stopRun();
		e->ignore();
	} else {
		//
		// quit the application but ask if there are unsaved changes.
		// Async (RULE 2): closeAllPrograms()'s confirmation dialogs can no
		// longer be waited on synchronously here (QDialog::exec() never
		// returns on the WASM main thread without Asyncify) -- always
		// ignore this event immediately, and the completion callback
		// performs the actual quit once the user has answered. Same
		// two-step pattern Qt's own docs use for async closeEvent handling.
		e->ignore();
		closeAllPrograms([this](bool doquit) {
			if (doquit) {
				// save current screen posision, visibility and floating
				saveCustomizations();
				quitConfirmed = true;
				QTimer::singleShot(0, qApp, SLOT(quit()));
				// close app as soon as the event loop is idle instead of using qApp->quit() to allow dispach of other events
				// This prevent app to not closing properly in rare situations like:
				// Interpreter emit() a blocking function in Controller (using QWaitCondition or BlockingQueuedConnection).
				// User request to close app while function is runnig in main loop. So, closeEvent is put in queue.
				// Function ends and return control to Interpreter. Interpreter request to run another code in main loop using emit().
				// Instead of running this code, the previous closeEvent() from queue is run.
				// Using qApp->quit() this will block forever Interpreter (never return), so, i->wait() will never return.
				// This is an old issue. It takes me a lot to manage it. (Florin)
			}
			// else not quitting -- this event was already ignored above
		});
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
    
    // Change display of word state
	if (runState==RUNSTATESTOP) statusBar()->showMessage(tr("Ready"));
	if (runState==RUNSTATERUN) statusBar()->showMessage(tr("Running"));
	if (runState==RUNSTATEDEBUG) statusBar()->showMessage(tr("Debug"));
	if (runState==RUNSTATESTOPING) statusBar()->showMessage(tr("Stoping"));
	if (runState==RUNSTATERUNDEBUG) statusBar()->showMessage(tr("Running in Debug"));
	

    updateRecent();
}

//Check for an update
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
        QRegularExpression rx("\\d+\\.\\d+\\.\\d+\\.\\d+");
        QString siteversion = rx.match(filename).captured(0);
        QString thisversion = rx.match(VERSION).captured(0);
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


QFont MainWindow::defaultEditorFont() {
    // The system's own fixed-width face (Consolas/Courier New on Windows, the
    // desktop monospace elsewhere) sized to visually match the rest of the UI.
    // The editor must stay monospace -- it is code -- so only the *size* follows
    // the system UI font, which is the part that was out of step with the menus.
    //
    // The UI font's size may be carried as points or as pixels: on Windows the
    // app font is derived from lfMessageFont (see Main.cpp), a pixel height, so
    // QApplication::font() often has a pixelSize and no valid pointSizeF --
    // pointSizeF() then returns -1. Reading only pointSizeF left the editor
    // stuck at the fixed face's own tiny default, never tracking the system
    // size. Copy whichever the UI font actually uses.
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont ui = QApplication::font();
    if (ui.pointSizeF() > 0) {
        f.setPointSizeF(ui.pointSizeF());
    } else if (ui.pixelSize() > 0) {
        f.setPixelSize(ui.pixelSize());
    }

    // At an equal nominal size a monospace face has a smaller x-height than the
    // proportional UI font, so the editor still reads as smaller than the menus
    // even though the point sizes match. Scale it up by the ratio of x-heights
    // (as CSS font-size-adjust does) so the two look the same visual size. Only
    // ever grow, and cap the growth so an unusual face cannot balloon.
    const QFontMetricsF uiM(ui), fM(f);
    if (fM.xHeight() > 0.0 && uiM.xHeight() > fM.xHeight()) {
        const qreal ratio = qMin(uiM.xHeight() / fM.xHeight(), 1.5);
        if (f.pointSizeF() > 0) {
            f.setPointSizeF(f.pointSizeF() * ratio);
        } else if (f.pixelSize() > 0) {
            f.setPixelSize(qRound(f.pixelSize() * ratio));
        }
    }
    return f;
}

void MainWindow::dialogFontSelect() {
    bool ok;
    editorFont = QFontDialog::getFont(&ok, editorFont, this, QString(), QFontDialog::MonospacedFonts);
    if (ok) {
        // An explicit choice pins the font: stop re-deriving it from the system.
        // Written out by saveCustomizations() along with the font itself.
        fontUserSet = true;
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
// Suspending the watch around the save is BasicEdit::writeFile()'s job, not
// ours: the overwrite prompt is non-modal (WASM RULE 2), so saveProgram()
// returns before the user has even answered it, and bracketing the *call* here
// re-armed the watcher while the write was still pending -- the editor then saw
// its own save land and asked whether to reload the file it had just written.
void MainWindow::activeEditorSaveProgram(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        e->saveProgram();
    }
}
void MainWindow::activeEditorSaveAsProgram(){
    BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
    if(e){
        // Save As detaches this editor from its current file, so stop watching
        // that one; the new path gets watched by writeFile()'s watchFile().
        if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->removePath(e->filename);
        e->saveAsProgram();
    }
}

void MainWindow::unwatchFile(QString fn){
    if(fileSystemWatcher && !fn.isEmpty()) fileSystemWatcher->removePath(fn);
}

void MainWindow::watchFile(QString fn){
    if(fileSystemWatcher && !fn.isEmpty() && !fileSystemWatcher->files().contains(fn)){
        fileSystemWatcher->addPath(fn);
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
        // The actual close, shared by the "no unsaved changes" fast path and
        // the confirmation dialog's Yes branch.
        auto doCloseEditor = [this, e]() {
            if(fileSystemWatcher && !(e->filename.isEmpty())) fileSystemWatcher->removePath(e->filename);
            e->deleteLater();
        };
        if (e->document()->isModified()) {
            // Async (RULE 2): QMessageBox::warning()'s exec() never returns on
            // the WASM main thread without Asyncify -- prompt non-modally and
            // close in the completion slot instead of blocking for the answer.
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->setIcon(QMessageBox::Warning);
            msgBox->setWindowTitle(tr("Program modifications have not been saved."));
            msgBox->setText(tr("Do you want to discard your changes?"));
            msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox->setDefaultButton(QMessageBox::No);
            QObject::connect(msgBox, &QMessageBox::finished, this, [doCloseEditor](int result){
                if (result == QMessageBox::Yes) doCloseEditor();
            });
            msgBox->setWindowModality(Qt::ApplicationModal);
            msgBox->show();
        } else {
            doCloseEditor();
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
        editor->slotWrap(edit_wrap_act->isChecked());
        outwin->slotWrap(edit_wrap_act->isChecked());
        // Parented to the editor's QTextDocument (see the QSyntaxHighlighter
        // base ctor), so it lives and dies with the editor when its tab is
        // closed. Nothing needs to hold the pointer.
        new EditSyntaxHighlighter(editor->document());
        // connect the signals
        QObject::connect(edit_whitespace_act, SIGNAL(toggled(bool)), editor, SLOT(slotWhitespace(bool)));
        QObject::connect(edit_wrap_act, SIGNAL(toggled(bool)), editor, SLOT(slotWrap(bool)));
        QObject::connect(edit_wrap_act, SIGNAL(toggled(bool)), outwin, SLOT(slotWrap(bool)));
        QObject::connect(editor, SIGNAL(changeStatusBar(QString)), this, SLOT(updateStatusBar(QString)));
        QObject::connect(editor, SIGNAL(updateWindowTitle(BasicEdit*)), this, SLOT(updateWindowTitle(BasicEdit*)));
        QObject::connect(this, SIGNAL(setEditorRunState(int)), editor, SLOT(setEditorRunState(int)));
        if(guiState!=GUISTATERUN){
            QObject::connect(editor, SIGNAL(updateEditorButtons()), this, SLOT(updateEditorButtons()));
            QObject::connect(editor, SIGNAL(addFileToRecentList(QString)), this, SLOT(addFileToRecentList(QString)));
            QObject::connect(editor, SIGNAL(setCurrentEditorTab(BasicEdit*)), this, SLOT(setCurrentEditorTab(BasicEdit*)));
            QObject::connect(this, SIGNAL(saveAllStep(int)), editor, SLOT(saveAllStep(int)));
            QObject::connect(editor, SIGNAL(unwatchFile(QString)), this, SLOT(unwatchFile(QString)));
            QObject::connect(editor, SIGNAL(watchFile(QString)), this, SLOT(watchFile(QString)));
            if(fileSystemWatcher) QObject::connect(fileSystemWatcher, SIGNAL(fileChanged(QString)), editor, SLOT(fileChangedOnDiskSlot(QString)) );
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
#ifdef Q_OS_WASM
    // The browser has no real filesystem path to hand back (RULE 2 also
    // applies here -- getOpenFileName()'s underlying dialog would have the
    // same never-returns problem as exec()) -- getOpenFileContent() instead
    // hands the picked file's name and content directly to a callback.
    QFileDialog::getOpenFileContent(
        QObject::tr("BASIC-256 file ") + "(*.kbs);;" + QObject::tr("Any File ") + "(*.*)",
        [this](const QString &fileName, const QByteArray &fileContent) {
            if (!fileName.isEmpty()) {
                loadFileContent(fileName, fileContent);
            }
        }
    );
#else
    QString s = QFileDialog::getOpenFileName(this, QObject::tr("Open a file"), ".", QObject::tr("BASIC-256 file ") + "(*.kbs);;" + QObject::tr("Any File ") + "(*.*)");
    loadFile(s);
#endif
}

#ifdef Q_OS_WASM
void MainWindow::loadFileContent(QString fileName, const QByteArray &content) {
    // WASM counterpart to loadFile(): getOpenFileContent() hands us the
    // file's content directly, with no real path at all -- so there is no
    // QFile to read, no path to compare against already-open tabs, no
    // mime-type/extension confirmation (the browser's own picker already
    // filtered by the accepted types), and no fileSystemWatcher path to
    // add. filename is deliberately left empty afterwards: browsers can't
    // silently overwrite a previously-downloaded file, so every Save must
    // always go through saveFileContent(), the same as a brand new file.
    bool replaceEmptyDoc = false;
    BasicEdit *neweditor = nullptr;
    if (untitledNumber == 2) {
        BasicEdit *e = (BasicEdit*)editwintabs->currentWidget();
        if (e) {
            if (e->filename.isEmpty() && !e->document()->isModified()) {
                neweditor = e;
                replaceEmptyDoc = true;
            }
        }
    }
    if (!replaceEmptyDoc) neweditor = newEditor(fileName);
    editwin = neweditor;
    neweditor->filename = "";
    neweditor->path = "";
    neweditor->title = fileName;

    updateStatusBar(QObject::tr("Loading file..."));
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    neweditor->setPlainText(QString::fromUtf8(content));
    neweditor->document()->setModified(false);
    setWindowTitle(fileName);
    QApplication::restoreOverrideCursor();
    updateStatusBar(QObject::tr("Ready."));

    //add tab and make it active
    if (!replaceEmptyDoc) {
        int i = editwintabs->addTab(neweditor, neweditor->title);
        editwintabs->setTabIcon(i, basicIcons->documentIcon);
        editwintabs->setCurrentIndex(i);
    } else {
        neweditor->updateTitle();
    }
}

void MainWindow::hidePlayerChrome() {
    // The URL-launched player (WasmLaunch) shows its output and nothing else --
    // an embedded demo has no use for a menu bar offering "Open Example..." and
    // "Exit", nor for the output toolbars' zoom/save/clear buttons.
    //
    // Called for the player modes (?mode=graph|text|app); ide/edit are meant to
    // be worked in and keep the full furniture. Both output toolbars are hidden
    // unconditionally: graph mode never shows the text one and text mode never
    // shows the graphics one, so the redundant call is a no-op rather than a
    // reason to branch on guiState here.
    //
    // Deliberately NOT folded into configureGuiState(): that would also strip
    // desktop -g/-t/-a, a different use case which keeps its chrome (maintainer
    // decision 2026-07-12). configureGuiState() hides the *main* toolbar but
    // never touches the output ones, which is why they are still up by the time
    // we get here.
    //
    // Safe to persist: saveCustomizations() keys every setting by guiState, so
    // what this writes lands under the player mode's keys, never the normal
    // IDE's (0) -- and on wasm those modes are only ever reached through here.
    menuBar()->hide();
    statusBar()->hide();
    graphwin_toolbar_visible_act->setChecked(false);
    graphwin_widget->slotShowToolBar(false);
    outwin_toolbar_visible_act->setChecked(false);
    outwin_widget->slotShowToolBar(false);
}
#endif

void MainWindow::openExample() {
#ifdef Q_OS_WASM
    // DemoWASM/examples.qrc (CMakeLists.txt, EMSCRIPTEN-only) bundles a curated,
    // self-contained subset of Examples/ under this prefix, grouped into category
    // folders (Games/, Demo/, Simulations/...). Resource reads are synchronous
    // (compiled into the binary), unlike getOpenFileContent() -- no async callback
    // is needed for the read itself, only for the picker, which must not block:
    // RULE 2, exec() never returns on the WASM main thread. Hence open(), not
    // exec(), and the file is loaded from the accepted() handler.
    //
    // Recursive listing: QDir::entryList() does not descend, so with a nested tree
    // it would return nothing at all and the picker would silently do nothing.
    QStringList files;
    const QString prefix = QStringLiteral(":/examples/");
    QDirIterator it(":/examples", QStringList() << "*.kbs", QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        files << it.filePath().mid(prefix.size());
    }
    if (files.isEmpty()) return;
    files.sort(Qt::CaseInsensitive);

    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Open Example"));

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(new QLabel(tr("Choose an example program:"), dialog));

    QTreeWidget *tree = new QTreeWidget(dialog);
    tree->setHeaderHidden(true);
    tree->setUniformRowHeights(true);
    layout->addWidget(tree);

    // Build the tree from the relative paths. Keyed on the full prefix rather
    // than the last path component, so two categories could hold a subfolder of
    // the same name without colliding -- and so a deeper tree still works, even
    // though DemoWASM/ is one level deep today.
    QHash<QString, QTreeWidgetItem*> nodes;
    for (const QString &rel : std::as_const(files)) {
        QStringList parts = rel.split(QLatin1Char('/'));
        const QString name = parts.takeLast();
        QTreeWidgetItem *parent = nullptr;
        QString path;
        for (const QString &part : std::as_const(parts)) {
            path = path.isEmpty() ? part : path + QLatin1Char('/') + part;
            QTreeWidgetItem *node = nodes.value(path, nullptr);
            if (!node) {
                node = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
                node->setText(0, part);
                // A category is a heading, not something you can open. Leaving it
                // selectable would let the Open button act on a folder.
                node->setFlags(node->flags() & ~Qt::ItemIsSelectable);
                nodes.insert(path, node);
            }
            parent = node;
        }
        QTreeWidgetItem *leaf = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
        leaf->setText(0, name);
        leaf->setData(0, Qt::UserRole, rel);   // the full path is what we load
    }
    // Start collapsed: the whole point of the tree is that the handful of
    // categories fit on screen at once. Expanding all 70-odd programs up front
    // would just be the flat list again, with extra indentation.
    tree->collapseAll();

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Open | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttons);

    // Only a program can be opened, never a category.
    QPushButton *openButton = buttons->button(QDialogButtonBox::Open);
    openButton->setEnabled(false);
    QObject::connect(tree, &QTreeWidget::currentItemChanged, dialog,
                     [openButton](QTreeWidgetItem *current, QTreeWidgetItem *){
        openButton->setEnabled(current && !current->data(0, Qt::UserRole).toString().isEmpty());
    });
    QObject::connect(tree, &QTreeWidget::itemDoubleClicked, dialog,
                     [dialog](QTreeWidgetItem *item, int){
        if (item && !item->data(0, Qt::UserRole).toString().isEmpty()) dialog->accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    QObject::connect(dialog, &QDialog::accepted, this, [this, tree](){
        QTreeWidgetItem *item = tree->currentItem();
        if (!item) return;
        const QString relPath = item->data(0, Qt::UserRole).toString();
        if (relPath.isEmpty()) return;          // a category, not a program
        QFile f(":/examples/" + relPath);
        if (f.open(QIODevice::ReadOnly)) {
            // Tab title is the bare file name -- the category is how you *found*
            // the program, not part of what it is called.
            loadFileContent(QFileInfo(relPath).fileName(), f.readAll());
            f.close();
        }
    });

    dialog->resize(420, 480);
    // Non-modal (RULE 2): exec() never returns on the WASM main thread.
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->open();
#else
    // Desktop: browse the bundled Examples/ directory with the normal file
    // dialog. Inside an AppImage the app runs from an ephemeral squashfs mount,
    // so the only reliable way to reach the bundled examples is a path resolved
    // relative to the executable -- probe the layouts the packaging scripts
    // produce and open the first that exists.
    const QString appDir = QCoreApplication::applicationDirPath();
    QString dir;
    const QStringList candidates = {
        appDir + "/Examples",                    // Windows / Linux zip & tar.gz (Examples beside the binary)
        appDir + "/../share/basic256/Examples",  // Linux AppImage (binary in usr/bin)
        appDir + "/../../../Examples",           // macOS .app (Examples beside the bundle)
    };
    for (const QString &c : std::as_const(candidates)) {
        if (QDir(c).exists()) { dir = QDir(c).absolutePath(); break; }
    }
    // No bundled Examples found (e.g. a bare dev build) -> let the dialog open
    // wherever the plain Open dialog would.
    if (dir.isEmpty()) dir = QStringLiteral(".");
    const QString s = QFileDialog::getOpenFileName(this, QObject::tr("Open an example"), dir,
        QObject::tr("BASIC-256 file ") + "(*.kbs);;" + QObject::tr("Any File ") + "(*.*)");
    loadFile(s);
#endif
}

bool MainWindow::loadFile(QString s) {
    s = s.trimmed();
    if (!s.isNull()) {
        bool doload = true;
        // On WASM these confirmations can't block-and-return on the main thread
        // (RULE 2), and loadFile() is effectively desktop-only there anyway
        // (WASM opens via getOpenFileContent()/loadFileContent()). Treat the
        // prompts like --silent: proceed rather than hang.
        bool skipLoadPrompts = (guiState == GUISTATESILENT);
#ifdef Q_OS_WASM
        skipLoadPrompts = true;
#endif
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
                    // --silent: there is no user to confirm anything and no window
                    // should ever appear, so always proceed rather than showing a
                    // modal dialog that would otherwise hang the process forever.
                    if (!(mime.inherits("text/plain") && !(fi.fileName().endsWith(".kbs",Qt::CaseInsensitive) && fi.size()==0))) {
                        doload = skipLoadPrompts || ( QMessageBox::Yes == QMessageBox::warning(this, QObject::tr("Load File"),
                            QObject::tr("It does not seem to be a text file.")+ "\n" + QObject::tr("Load it anyway?"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No));
                    }else if (!fi.fileName().endsWith(".kbs",Qt::CaseInsensitive)) {
                        doload = skipLoadPrompts || ( QMessageBox::Yes == QMessageBox::warning(this, QObject::tr("Load File"),
                            QObject::tr("You're about to load a file that does not end with the .kbs extension.")+ "\n" + QObject::tr("Load it anyway?"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No));
                    }
                    if (doload) {
                        //replace empty document created by default (if exists)
                        bool replaceEmptyDoc = false;
                        BasicEdit *neweditor = nullptr;
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
                    // --silent: skip the modal (no user to dismiss it, no window
                    // should ever appear); Main.cpp reports the load failure on
                    // stderr and exits non-zero instead.
                    if (guiState != GUISTATESILENT) {
                        QMessageBox::warning(this, QObject::tr("Load File"),
                            QObject::tr("Unable to open program file")+" \""+s+"\".\n"+QObject::tr("File permissions problem or file open by another process."),
                            QMessageBox::Ok, QMessageBox::Ok);
                    }
                }
            } else {
                if (guiState != GUISTATESILENT) {
                    QMessageBox::warning(this, QObject::tr("Load File"),
                        QObject::tr("Program file does not exist.")+" \""+s+QObject::tr("\"."),
                        QMessageBox::Ok, QMessageBox::Ok);
                }
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

void MainWindow::closeAllProgramsSlot(){
    closeAllPrograms([](bool){});
}

void MainWindow::closeAllPrograms(std::function<void(bool)> onDone){
    QString list;
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
    //if there are no unsaved files, nothing to ask -- finish immediately
    if(count==0){
        finishCloseAllPrograms(true, onDone);
        return;
    }
    //there are unsaved files -- ask what to do. Async (RULE 2):
    //QDialog::exec() never returns on the WASM main thread without
    //Asyncify, so show the prompt non-modally and continue in the
    //completion slot instead of blocking here for the answer.
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle(QObject::tr("Save changes?"));
    if(count==1){
        msgBox->setText(QObject::tr("The following file have unsaved changes:"));
    }else{
        msgBox->setText(QObject::tr("The following files have unsaved changes:"));
    }
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setInformativeText(list);
    msgBox->setStandardButtons( (count==1?QMessageBox::Save:QMessageBox::SaveAll) | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox->setDefaultButton(QMessageBox::Cancel);
    QObject::connect(msgBox, &QMessageBox::finished, this, [this, onDone](int doclose){
        if(doclose==QMessageBox::SaveAll || doclose==QMessageBox::Save){
            saveAll();
        }else if(doclose==QMessageBox::Cancel){
            onDone(false);
            return;
        }

        //double check for unsaved files - user press [Cancel] at saving time
        int count2 = 0;
        for(int i=0; i<editwintabs->count(); i++){
            BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
            if(e){
                if(e->document()->isModified()){
                    count2++;
                }
            }
        }
        if(count2>0){
            QMessageBox *msgBox2 = new QMessageBox(this);
            msgBox2->setAttribute(Qt::WA_DeleteOnClose);
            msgBox2->setWindowTitle(QObject::tr("Unsaved files"));
            msgBox2->setText(QObject::tr("There are unsaved files.")+ "\n" + QObject::tr("Do you really want to discard your changes?"));
            msgBox2->setIcon(QMessageBox::Warning);
            msgBox2->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox2->setDefaultButton(QMessageBox::No);
            QObject::connect(msgBox2, &QMessageBox::finished, this, [this, onDone](int result){
                finishCloseAllPrograms(result==QMessageBox::Yes, onDone);
            });
            // open() forces window-modal, unlike exec()'s old application-modal
            // default; force it back so the main window's close button can't
            // re-enter this flow while the dialog is up.
            msgBox2->setWindowModality(Qt::ApplicationModal);
            msgBox2->show();
        }else{
            finishCloseAllPrograms(true, onDone);
        }
    });
    // See msgBox2 above: force application-modal instead of open()'s window-modal.
    msgBox->setWindowModality(Qt::ApplicationModal);
    msgBox->show();
}

void MainWindow::finishCloseAllPrograms(bool doit, std::function<void(bool)> onDone){
    if(doit){
        rc->stopRun();
        for(int i=editwintabs->count()-1; i>=0; i--){
            BasicEdit *e = (BasicEdit*)editwintabs->widget(i);
            e->deleteLater();
        }
    }
    onDone(doit);
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
