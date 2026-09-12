/** Copyright (C) 2014, James Reneau.
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


#ifndef __SLEEPER_H
#define __SLEEPER_H

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif
#include <chrono>
#include <condition_variable>
#include <mutex>

class Sleeper
{
public:
	Sleeper();

	// All three block until the deadline, or until wake() is called from
	// another thread, whichever comes first. They return true when the full
	// time was slept and false when wake() cut it short.
	bool sleepUntil(std::chrono::steady_clock::time_point finish);
	bool sleepMS(long int ms);
	bool sleepSeconds(double s);

	// Uninterruptable fixed-length sleep - still used where a caller wants a
	// short unconditional settling delay (see BasicMediaPlayer).
	void sleepRQM(long int ms);

	void wake();
	void clearWake();

private:
	std::mutex sleepmutex;
	std::condition_variable sleepcond;
	bool wakesleeper;

};

#endif
