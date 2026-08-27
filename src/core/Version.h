/** Copyright (C) 2006, Ian Paul Larsen.
 **
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


#ifndef __VERSION
#define __VERSION

// VERSION is normally supplied by CMake (see CMakeLists.txt), derived from
// the branch being built, so the About box always matches the build. This
// is only the fallback for builds that don't go through CMake's derivation
// (e.g. an IDE opening this header directly without a configure step).
#ifndef VERSION
#define VERSION "2.1.1"
#endif
// What the language's version function returns, encoded as
// major*1000000 + minor*10000 + patch*100 + sub, so 2.0.0.11 = 2000011 and
// 2.1.2.0 = 2010200. Programs gate on it (TestSuite/testsuite.kbs decodes
// exactly this scheme), so keep it in step with VERSIONPRODUCT below.
//
// Unlike VERSION above, this does NOT follow the branch name: a program
// asking version() on a 2.1.2 beta build should be told 2.1.2, so these two
// are bumped by hand when a new patch line opens.
#define VERSIONSIGNATURE  2010200
#define VERSIONPRODUCT 2,1,2,0

#endif
