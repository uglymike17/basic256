/** Copyright (C) 2014, James Reneau.
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

#include "Constants.h"
#include "Sleeper.h"
#include <chrono>


Sleeper::Sleeper() {
	wakesleeper=false;
}

void Sleeper::wake() {
	// signal the sleeper to wake
	wakesleeper=true;
}

bool Sleeper::sleepMS(long int ms) {
	// interruptable - return true if NOT interrupted
	std::chrono::steady_clock::time_point finish;
	long int remainingms;
	finish = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
	wakesleeper=false;
	while (std::chrono::steady_clock::now() < finish && !wakesleeper) {
		remainingms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - std::chrono::steady_clock::now()).count();
		if(remainingms > SLEEP_GRANULE){
			ms -= SLEEP_GRANULE;
			sleepRQM(SLEEP_GRANULE);
		}else{
			sleepRQM(remainingms);
			break;
		}
	}
	return !wakesleeper;
}

void Sleeper::sleepRQM(long int ms) {
// sleep ms miliseconds - an uninterruptable quantum moment
#ifdef WIN32
		Sleep(ms);
#else
        int s=0;
		if (ms>=1000) {
			s = (ms/1000);
			ms %= 1000;
		}
		struct timespec tim;
        tim.tv_sec = s;
		tim.tv_nsec = ms * 1000000L;
		nanosleep(&tim, NULL);
#endif
}

void Sleeper::sleepSeconds(double s) {
	long int ms = s * 1000L;
	sleepMS(ms);
}

	
