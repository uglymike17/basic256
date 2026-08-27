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

#include "MenuIndicatorStyle.h"

#include <QAction>
#include <QFont>
#include <QMenu>
#include <QPainter>
#include <QPaintDevice>
#include <QPixmap>
#include <QPixmapCache>
#include <QStyleOption>
#include <QWidget>

bool MenuIndicatorStyle::menuHasCheckableItem(const QWidget *widget) {
	const QMenu *menu = qobject_cast<const QMenu *>(widget);
	if (!menu) return false;
	const QList<QAction *> actions = menu->actions();
	for (const QAction *action : actions) {
		if (action->isCheckable()) return true;
	}
	return false;
}

int MenuIndicatorStyle::indicatorBox(const QStyleOptionMenuItem &item, const QWidget *widget) const {

	const bool exclusive = (item.checkType == QStyleOptionMenuItem::Exclusive);

	int box = proxy()->pixelMetric(exclusive ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth,
								   &item, widget);
	return capToSlot(box, item, widget);
}

int MenuIndicatorStyle::indicatorColumn(const QStyleOptionMenuItem &item, const QWidget *widget) const {

	// Widest of the two indicators, so every entry in a menu is measured
	// against the same column whether it holds a check box, a radio button or
	// nothing at all -- otherwise the text of a radio item and the text of a
	// check item in one menu would start at different places.
	int box = qMax(proxy()->pixelMetric(PM_IndicatorWidth, &item, widget),
				   proxy()->pixelMetric(PM_ExclusiveIndicatorWidth, &item, widget));
	box = capToSlot(box, item, widget);

	// Half a box of clear space to the right of the indicator.
	return box + box / 2;
}

int MenuIndicatorStyle::naturalBox(const QStyleOptionMenuItem &item, const QWidget *widget) const {

	const bool exclusive = (item.checkType == QStyleOptionMenuItem::Exclusive);

	int box = proxy()->pixelMetric(exclusive ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth,
								   &item, widget);
	if (box <= 0) box = proxy()->pixelMetric(PM_SmallIconSize, &item, widget);
	return box > 0 ? box : 0;
}

void MenuIndicatorStyle::drawIndicator(const QStyleOptionMenuItem &item, const QWidget *widget,
									   const QRect &rect, QPainter *painter) const {

	Q_UNUSED(widget);

	const bool exclusive = (item.checkType == QStyleOptionMenuItem::Exclusive);

	QStyleOptionButton indicator;
	indicator.rect = rect;
	indicator.palette = item.palette;
	indicator.fontMetrics = item.fontMetrics;
	// Always drawn as enabled. A disabled item is dimmed by the base style
	// when it renders the icon (QIcon::Disabled); dimming it twice would
	// leave the box nearly invisible.
	indicator.state = State_Enabled | (item.checked ? State_On : State_Off);
	// No widget is passed: this is a check box being drawn, not the menu,
	// and a style that reads hover or animation state off the widget would
	// be reading it from the wrong one.
	proxy()->drawPrimitive(exclusive ? PE_IndicatorRadioButton : PE_IndicatorCheckBox,
						   &indicator, painter, nullptr);
}

int MenuIndicatorStyle::capToSlot(int box, const QStyleOptionMenuItem &item, const QWidget *widget) const {

	const int iconSize = proxy()->pixelMetric(PM_SmallIconSize, &item, widget);
	if (box <= 0) box = iconSize;

	// The pixmap is the box plus half a box of empty space (see indicatorIcon),
	// and the whole thing has to stay inside an icon slot or QIcon scales it
	// down and the box comes out blurred. That caps the box at two thirds of
	// the slot -- a little smaller than the indicator the style draws in a
	// dialog, which is the price of it sitting clear of the text.
	const int maxBox = (iconSize * 2) / 3;
	if (maxBox > 0 && box > maxBox) box = maxBox;

	return box > 0 ? box : 0;
}

void MenuIndicatorStyle::drawControl(ControlElement element, const QStyleOption *option,
									 QPainter *painter, const QWidget *widget) const {

	if (element == CE_MenuBarItem) {
		// The Windows 11 style paints every menu bar title at a hardcoded 10
		// points -- qwindows11style.cpp, CE_MenuBarItem, "newMbi.font.setPointSize(10)"
		// -- so File/Edit/View/... alone stay one fixed size while the rest of
		// the application follows the system text size (Main.cpp derives the
		// application font from it). It hands the text to QCommonStyle, which
		// draws it through proxy()->drawItemText(), and proxy() is this style:
		// note the menu bar for the length of this one item and drawItemText()
		// below puts the widget's own font back. Correcting it there leaves
		// every rectangle the style worked out untouched -- only the size of
		// the glyphs changes -- and costs nothing under a style that never
		// substituted a font in the first place.
		const QWidget *previous = menubar;
		menubar = widget;
		QProxyStyle::drawControl(element, option, painter, widget);
		menubar = previous;
		return;
	}

	if (element == CE_MenuItem) {
		const QStyleOptionMenuItem *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
		if (item && item->menuItemType == QStyleOptionMenuItem::Normal
				 && menuHasCheckableItem(widget)) {

			QStyleOptionMenuItem indicated(*item);

			// A menu of nothing but checkable, icon-less entries -- View/Theme,
			// View/Graphics Window Zoom, the open programs in Window -- reports
			// maxIconWidth as 0, so the base style starts the text at the left
			// edge and an injected pixmap lands on top of it. Claim the column
			// the indicator needs; sizeFromContents() widens the menu to match.
			// Every entry of such a menu is given the same column, including
			// the plain commands, so their text stays in one line down the menu.
			const int column = indicatorColumn(*item, widget);
			if (column > indicated.maxIconWidth) indicated.maxIconWidth = column;

			if (item->checkType != QStyleOptionMenuItem::NotCheckable) {
				qreal dpr = 1.0;
				if (painter && painter->device()) dpr = painter->device()->devicePixelRatioF();

				indicated.icon = indicatorIcon(*item, widget, dpr);
				// The pixmap now carries the state, so hand the base style what
				// looks like an ordinary item: styles that tick a checked item,
				// or that draw a box of their own for an icon-less one, would
				// otherwise paint a second indicator over ours.
				indicated.checkType = QStyleOptionMenuItem::NotCheckable;
				indicated.checked = false;
			}

			QProxyStyle::drawControl(element, &indicated, painter, widget);
			return;
		}
	}

	QProxyStyle::drawControl(element, option, painter, widget);
}

void MenuIndicatorStyle::drawItemText(QPainter *painter, const QRect &rect, int flags,
									  const QPalette &pal, bool enabled, const QString &text,
									  QPalette::ColorRole textRole) const {

	if (menubar && painter) {
		// A menu bar title, so draw it in the menu bar's own font rather than
		// whatever the base style left on the painter (see drawControl).
		const QFont substituted = painter->font();
		painter->setFont(menubar->font());
		QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
		painter->setFont(substituted);
		return;
	}

	QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
}

QSize MenuIndicatorStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
										   const QSize &contentsSize, const QWidget *widget) const {

	QSize size = QProxyStyle::sizeFromContents(type, option, contentsSize, widget);

	if (type == CT_MenuBarItem && !contentsSize.isEmpty()) {
		// The same style fixes the height of a menu bar item at 32 logical
		// pixels whatever the text in it, so at a large system text size the
		// title -- now drawn at that size, see drawItemText() -- no longer fits
		// the bar and is clipped top and bottom. contentsSize is the size of
		// the text itself; keep the padding the style asked for around it, and
		// only ever grow, so the bar is left alone at the sizes where it fits.
		const int padding = 10;
		size.setHeight(qMax(size.height(), contentsSize.height() + padding));
	}

	if (type == CT_MenuItem) {
		const QStyleOptionMenuItem *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
		if (item && item->menuItemType == QStyleOptionMenuItem::Normal
				 && menuHasCheckableItem(widget)) {
			// Matches the column drawControl() claims. Without this the item is
			// measured for the icon column it actually has -- none, in the
			// menus that need this most -- and the text is drawn into space the
			// menu never reserved, so it runs into the right-hand edge.
			const int column = indicatorColumn(*item, widget);
			if (column > item->maxIconWidth) size.rwidth() += column - item->maxIconWidth;
		}
	}

	return size;
}

QIcon MenuIndicatorStyle::indicatorIcon(const QStyleOptionMenuItem &item, const QWidget *widget,
										qreal devicePixelRatio) const {

	const bool exclusive = (item.checkType == QStyleOptionMenuItem::Exclusive);

	const int box = indicatorBox(item, widget);
	if (box <= 0) return QIcon();
	const int column = indicatorColumn(item, widget);
	// What the style would draw if the slot cap were not in the way. Part of
	// the cache key: two different naturals can cap to the same box, and they
	// do not produce the same pixmap.
	const int natural = naturalBox(item, widget);

	// The palette key covers a switch between the light and dark colour scheme,
	// which repaints the menus with different indicator colours.
	const QString key = QStringLiteral("basic256-menuindicator-")
			+ QString::number(box) + QLatin1Char('-')
			+ QString::number(column) + QLatin1Char('-')
			+ QString::number(natural) + QLatin1Char('-')
			+ QString::number(qRound(devicePixelRatio * 100)) + QLatin1Char('-')
			+ QLatin1Char(exclusive ? 'r' : 'c')
			+ QLatin1Char(item.checked ? '1' : '0') + QLatin1Char('-')
			+ QString::number(item.palette.cacheKey());

	QPixmap pixmap;
	if (!QPixmapCache::find(key, &pixmap)) {
		// As wide as the column, with the box against its left edge: the base
		// style centres a pixmap in the space it is given, so the empty
		// right-hand part carries the box itself half its own width to the left
		// of centre, away from the text that follows.
		pixmap = QPixmap(QSize(column, box) * devicePixelRatio);
		pixmap.setDevicePixelRatio(devicePixelRatio);
		pixmap.fill(Qt::transparent);

		QPainter pixmapPainter(&pixmap);
		if (natural > box) {
			// The style wants more room than the slot allows, and not every
			// style treats the rect it is handed as a limit: the Windows radio
			// button is drawn at its own size about the centre of that rect, so
			// a box smaller than it wants overflows the pixmap and the ring
			// comes out clipped -- as a bracket, since the box sits against the
			// pixmap's left edge. Give the style the size it asked for on a
			// surface that size, then scale the result into the slot. Costs a
			// little sharpness on the platforms that need it, and cannot
			// misplace the indicator whatever the style does inside its rect.
			QPixmap full(QSize(natural, natural) * devicePixelRatio);
			full.setDevicePixelRatio(devicePixelRatio);
			full.fill(Qt::transparent);

			QPainter fullPainter(&full);
			drawIndicator(item, widget, QRect(0, 0, natural, natural), &fullPainter);
			fullPainter.end();

			QPixmap fitted = full.scaled(QSize(box, box) * devicePixelRatio,
										 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			fitted.setDevicePixelRatio(devicePixelRatio);
			pixmapPainter.drawPixmap(0, 0, fitted);
		} else {
			// The indicator already fits. Drawn straight into the slot, exactly
			// as before, so the platforms this renders correctly on today are
			// left pixel for pixel alone.
			drawIndicator(item, widget, QRect(0, 0, box, box), &pixmapPainter);
		}
		pixmapPainter.end();

		QPixmapCache::insert(key, pixmap);
	}

	return QIcon(pixmap);
}
