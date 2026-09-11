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

#include "Constants.h"
#include "Sleeper.h"
#include <chrono>

#ifdef WIN32
#include <timeapi.h>
// timeBeginPeriod/timeEndPeriod live in winmm. Windows rounds every timed wait
// (Sleep, and the condition-variable wait below alike) UP to the next tick of
// the system timer, which defaults to 15.6ms -- so without this a "wait 16.7ms"
// for a 60fps frame becomes a 31ms wait and delivers 32fps. Asking for 1ms
// while a sleep is in progress makes the millisecond accuracy this class
// promises actually true rather than a side effect of whether some other part
// of the process (the audio device, typically) happens to be holding the timer
// down at the time. Since Windows 10 2004 the request is per-process, so it
// does not change the timer for anything else on the machine, and it is
// reference counted, so the nesting below is safe.
class TimerResolutionGuard {
public:
	TimerResolutionGuard() { timeBeginPeriod(1); }
	~TimerResolutionGuard() { timeEndPeriod(1); }
};
#endif


Sleeper::Sleeper() {
	wakesleeper=false;
}

void Sleeper::wake() {
	// signal the sleeper to wake - called from another thread
	{
		std::lock_guard<std::mutex> lock(sleepmutex);
		wakesleeper=true;
	}
	sleepcond.notify_all();
}

void Sleeper::clearWake() {
	// drop a wake signal nobody consumed - called once at the start of a run
	// so a stale stop from the previous run cannot shorten the first sleep
	std::lock_guard<std::mutex> lock(sleepmutex);
	wakesleeper=false;
}

bool Sleeper::sleepUntil(std::chrono::steady_clock::time_point finish) {
	// interruptable - return true if NOT interrupted
	//
	// wait_until parks on the OS until the deadline arrives or wake() signals,
	// so there is no polling loop: the deadline is honoured exactly (to the
	// timer resolution) and a wake() is acted on at once instead of at the end
	// of the current poll interval. The predicate also absorbs the spurious
	// wakeups wait_until is allowed to deliver.
	//
	// The flag is deliberately NOT cleared on entry. A wake() that lands just
	// before a sleep starts is a stop that must still be obeyed, so it is left
	// standing for this sleep to see and is consumed only once it has had an
	// effect.
#ifdef WIN32
	TimerResolutionGuard resolution;
#endif
	std::unique_lock<std::mutex> lock(sleepmutex);
	if (sleepcond.wait_until(lock, finish, [this]{ return wakesleeper; })) {
		wakesleeper=false;
		return false;
	}
	return true;
}

bool Sleeper::sleepMS(long int ms) {
	// interruptable - return true if NOT interrupted
	if (ms <= 0) return true;
	return sleepUntil(std::chrono::steady_clock::now() +
		std::chrono::milliseconds((long long) ms));
}

bool Sleeper::sleepSeconds(double s) {
	// interruptable - return true if NOT interrupted
	//
	// The seconds go straight to the clock's own (nanosecond) resolution: the
	// old route through a whole number of milliseconds truncated, so PAUSE .29
	// became 289ms and returned early, and on a 32-bit long any delay past
	// ~24.8 days overflowed and returned at once.
	if (!(s > 0)) return true;
	if (s > 86400.0) s = 86400.0;		// a day is as long as a PAUSE gets
	return sleepUntil(std::chrono::steady_clock::now() +
		std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<double>(s)));
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

	
