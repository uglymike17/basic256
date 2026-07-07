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


#ifndef __VERSION
#define __VERSION

// VERSION is normally supplied by CMake (see CMakeLists.txt), derived from
// the branch being built, so the About box always matches the build. This
// is only the fallback for builds that don't go through CMake's derivation
// (e.g. an IDE opening this header directly without a configure step).
#ifndef VERSION
#define VERSION "2.1.Alpha04"
#endif
#define VERSIONSIGNATURE  2001004
#define VERSIONPRODUCT 2,1,0,4

#endif
