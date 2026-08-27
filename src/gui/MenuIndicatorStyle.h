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

#ifndef __MENUINDICATORSTYLE_H
#define __MENUINDICATORSTYLE_H

#include <QIcon>
#include <QProxyStyle>

class QStyleOptionMenuItem;

// Paints a real check box -- or a radio button, for the members of an exclusive
// group such as View/Theme and View/Graphics Window Zoom -- in the left column
// of every checkable menu item, whether it is checked or not.
//
// Left to itself the native Windows style puts a bare tick there when an item is
// on and draws nothing at all when it is off, so entries like View/Wrap Long
// Lines are indistinguishable from plain commands until the user happens to
// switch one on. Fusion (Linux, and the browser build) does draw a box, but only
// for items that carry no icon of their own.
//
// Rather than guess where each style keeps that column, the item's icon is
// swapped for a pixmap of the indicator and the base style lays the item out as
// it always does: the indicator lands exactly where an icon would, at the size
// and alignment the platform intends. The swap is deliberate for the one
// checkable item that has an icon (View/Graphics Window Grid Lines) as well --
// a checkable item should read as checkable, and the icon still shows on the
// toolbar. Only menu painting is affected; nothing else in the application
// changes appearance.
//
// It also keeps the menu bar's own titles at the system text size -- see
// drawItemText() -- which the Windows 11 style otherwise ignores.
class MenuIndicatorStyle : public QProxyStyle
{
public:
	void drawControl(ControlElement element, const QStyleOption *option,
					 QPainter *painter, const QWidget *widget) const override;
	void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal,
					  bool enabled, const QString &text,
					  QPalette::ColorRole textRole = QPalette::NoRole) const override;
	QSize sizeFromContents(ContentsType type, const QStyleOption *option,
						   const QSize &contentsSize, const QWidget *widget) const override;

private:
	// The menu bar whose item is being painted right now, or nullptr when the
	// text about to be drawn is anything else. Set by drawControl() for the
	// length of one CE_MenuBarItem and read back in drawItemText(), which the
	// base style reaches through proxy() while it draws that item.
	mutable const QWidget *menubar = nullptr;

	static bool menuHasCheckableItem(const QWidget *widget);
	// Side of the square box, and the width of the column it is drawn in --
	// half a box wider, so the box sits left of the text rather than under it.
	int indicatorBox(const QStyleOptionMenuItem &item, const QWidget *widget) const;
	int indicatorColumn(const QStyleOptionMenuItem &item, const QWidget *widget) const;
	int capToSlot(int box, const QStyleOptionMenuItem &item, const QWidget *widget) const;
	// The size the style itself wants for the indicator, before the slot cap.
	int naturalBox(const QStyleOptionMenuItem &item, const QWidget *widget) const;
	void drawIndicator(const QStyleOptionMenuItem &item, const QWidget *widget,
					   const QRect &rect, QPainter *painter) const;
	QIcon indicatorIcon(const QStyleOptionMenuItem &item, const QWidget *widget,
						qreal devicePixelRatio) const;
};

#endif // __MENUINDICATORSTYLE_H
