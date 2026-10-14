/**
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

#ifndef __EDITORTHEME_H
#define __EDITORTHEME_H

#include <QColor>
#include <QString>

// Every colour the Edit pane, its gutter, and the Text Output pane draw with.
// These used to be literals spread across EditSyntaxHighlighter, BasicEdit and
// BasicOutput, all chosen for a white page -- which is why a dark desktop theme
// made them unreadable and Beta2 had to pin both panes to white.
//
// The Graphics Output window is deliberately NOT covered here and must stay
// white. BASIC-256 programs draw with explicit colours and assume a white page
// (CLG with no argument clears to transparent and the canvas shows white
// beneath), so theming it would change the rendered output of every existing
// program, and with it what ImgSave, ImageCopy and GetSlice produce.
struct EditorTheme {
	// text panes
	QColor background;
	QColor foreground;
	QColor selectionBackground;
	QColor selectionForeground;

	// syntax highlighting
	QColor keyword;
	QColor constant;
	QColor quote;
	QColor label;
	QColor number;
	QColor comment;

	// editor highlight bands
	QColor currentLine;			// edit mode
	QColor currentBlock;
	QColor readOnlyLine;		// while running, editing is forbidden
	QColor readOnlyBlock;
	QColor debugLine;			// stopped in the debugger
	QColor bracketMatch;

	// line number gutter
	QColor gutterBackground;
	QColor gutterCurrent;		// band behind the current paragraph
	QColor gutterHover;			// band under the mouse
	QColor gutterText;
	QColor gutterTextCurrent;
	QColor breakpoint;

	// text output pane
	QColor outputText;
	QColor outputError;

	// column header of the Variable Watch window
	QColor headerBackground;
	QColor headerText;
	QColor headerBorder;

	// Stylesheet for a QPlainTextEdit/QTextEdit painted in this theme. A
	// stylesheet is used rather than setPalette() because some styles ignore
	// the palette under a dark colour scheme.
	// backgroundOverride, when valid, replaces this theme's background. The
	// Text Output pane uses it for the TEXTBACKGROUND statement, so a program
	// can paint the whole pane without the theme being switched underneath it.
	QString paneStyleSheet(const QString &widgetClass, const QColor &backgroundOverride = QColor()) const;

	static const EditorTheme &light();
	static const EditorTheme &dark();

	// How the theme is chosen. System follows the desktop colour scheme and is
	// the default; Light and Dark pin it regardless. Stored as these numbers,
	// so do not renumber them.
	enum Mode { System = 0, Light = 1, Dark = 2 };

	// The theme the panes should paint with right now.
	static const EditorTheme &current();

	static Mode mode();
	static void setMode(Mode m);

	// Resolves System against the desktop; never returns System.
	static Mode effectiveMode();

	// True when this Qt build can report the desktop colour scheme. On older Qt
	// there is nothing to follow, so System resolves to Light.
	static bool systemSchemeAvailable();
};

#endif	// __EDITORTHEME_H
