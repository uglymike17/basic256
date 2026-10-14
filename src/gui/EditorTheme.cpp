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

#include "EditorTheme.h"

#include <QGuiApplication>
#include <QStyleHints>

#include "Settings.h"

// Loaded from the stored setting on first use rather than at start up, so the
// panes get the right colours no matter how early they are constructed.
static EditorTheme::Mode s_mode = EditorTheme::System;
static bool s_modeLoaded = false;

QString EditorTheme::paneStyleSheet(const QString &widgetClass, const QColor &backgroundOverride) const {
	const QColor bg = backgroundOverride.isValid() ? backgroundOverride : background;
	return widgetClass + "{"
		"  background-color:" + bg.name() + ";"
		"  color:" + foreground.name() + ";"
		"  selection-background-color:" + selectionBackground.name() + ";"
		"  selection-color:" + selectionForeground.name() + ";"
		"}";
}

const EditorTheme &EditorTheme::light() {
	// Exactly the colours the panes used before the theme was extracted, so
	// the light scheme is pixel for pixel what Beta2 shipped.
	static const EditorTheme t = []{
		EditorTheme e;

		e.background = QColor("#ffffff");
		e.foreground = QColor("#000000");
		e.selectionBackground = QColor("#c0d8f0");
		e.selectionForeground = QColor("#000000");

		e.keyword = QColor(Qt::darkBlue);
		e.constant = QColor(Qt::darkCyan);
		e.quote = QColor(Qt::magenta);
		e.label = QColor(Qt::blue);
		e.number = QColor(Qt::darkMagenta);
		e.comment = QColor(Qt::darkGreen);

		e.currentLine = QColor(Qt::yellow).lighter(165);
		e.currentBlock = QColor(Qt::yellow).lighter(190);
		e.readOnlyLine = QColor(Qt::red).lighter(175);
		e.readOnlyBlock = QColor(Qt::red).lighter(190);
		e.debugLine = QColor(Qt::green).lighter(175);
		e.bracketMatch = QColor(Qt::green).lighter(165);

		e.gutterBackground = QColor(Qt::lightGray);
		e.gutterCurrent = QColor(Qt::lightGray).lighter(110);
		e.gutterHover = QColor(Qt::lightGray).lighter(104);
		e.gutterText = QColor(Qt::black);
		e.gutterTextCurrent = QColor(Qt::blue);
		e.breakpoint = QColor(Qt::red);

		e.outputText = QColor(Qt::black);
		e.outputError = QColor(Qt::red);

		e.headerBackground = QColor("#f0f0f0");
		e.headerText = QColor(Qt::black);
		e.headerBorder = QColor("#c0c0c0");

		return e;
	}();
	return t;
}

const EditorTheme &EditorTheme::dark() {
	// Hues follow the light scheme so code still reads as BASIC-256, lightened
	// enough to stay legible on a near-black page. The highlight bands are dark
	// and low saturation so the text on top of them keeps its contrast.
	static const EditorTheme t = []{
		EditorTheme e;

		e.background = QColor("#1e1e1e");
		e.foreground = QColor("#d4d4d4");
		e.selectionBackground = QColor("#264f78");
		e.selectionForeground = QColor("#ffffff");

		e.keyword = QColor("#6fa8ff");
		e.constant = QColor("#4ec9b0");
		e.quote = QColor("#ff79c6");
		e.label = QColor("#9cc4ff");
		e.number = QColor("#c792ea");
		e.comment = QColor("#7fbf7f");

		e.currentLine = QColor("#3d3d2a");
		e.currentBlock = QColor("#2f2f22");
		e.readOnlyLine = QColor("#4a2a2a");
		e.readOnlyBlock = QColor("#3a2424");
		e.debugLine = QColor("#24402a");
		e.bracketMatch = QColor("#2f5f3f");

		e.gutterBackground = QColor("#2b2b2b");
		e.gutterCurrent = QColor("#3d3d3d");
		e.gutterHover = QColor("#343434");
		e.gutterText = QColor("#909090");
		e.gutterTextCurrent = QColor("#6fa8ff");
		e.breakpoint = QColor("#ff5555");

		e.outputText = QColor("#d4d4d4");
		e.outputError = QColor("#ff6b6b");

		e.headerBackground = QColor("#2b2b2b");
		e.headerText = QColor("#d4d4d4");
		e.headerBorder = QColor("#3d3d3d");

		return e;
	}();
	return t;
}

bool EditorTheme::systemSchemeAvailable() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	return true;
#else
	return false;
#endif
}

EditorTheme::Mode EditorTheme::mode() {
	if (!s_modeLoaded) {
		SETTINGS;
		const int m = settings.value(SETTINGSTHEME, SETTINGSTHEMEDEFAULT).toInt();
		s_mode = (m == Light || m == Dark) ? (Mode)m : System;
		s_modeLoaded = true;
	}
	return s_mode;
}

void EditorTheme::setMode(Mode m) {
	s_mode = m;
	s_modeLoaded = true;
}

EditorTheme::Mode EditorTheme::effectiveMode() {
	const Mode m = mode();
	if (m != System) return m;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	// QStyleHints::colorScheme() is Qt 6.5 and later. It reports Unknown when
	// the platform cannot say, which we treat as light -- the scheme every
	// BASIC-256 release before this one assumed. styleHints() needs a live
	// QGuiApplication, so guard against being asked this too early.
	if (QGuiApplication::instance() &&
		QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) return Dark;
#endif
	return Light;
}

const EditorTheme &EditorTheme::current() {
	return effectiveMode() == Dark ? dark() : light();
}
