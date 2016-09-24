
#include "Timer.h"
#include <stddef.h>
#include <fcntl.h>
#include <sys/time.h>


Timer::Timer(void)
{
}

long long Timer::get_milliseconds() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time

    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}

int8_t Timer::every(long long period, void(*callback)(), int repeatCount)
{
	int8_t i = findFreeEventIndex();
	if (i == -1) return -1;

	_events[i].eventType = EVENT_EVERY;
	_events[i].period = period;
	_events[i].repeatCount = repeatCount;
	_events[i].callback = callback;
    _events[i].lastEventTime = get_milliseconds();
	_events[i].count = 0;
	return i;
}

int8_t Timer::every(long long period, void(*callback)())
{
	return every(period, callback, -1); // - means forever
}

int8_t Timer::after(long long period, void(*callback)())
{
	return every(period, callback, 1);
}


void Timer::stop(int8_t id)
{
	if (id >= 0 && id < MAX_NUMBER_OF_EVENTS) {
		_events[id].eventType = EVENT_NONE;
	}
}

void Timer::update(void)
{
    long long now = get_milliseconds();
	update(now);
}

void Timer::update(long long now)
{
	for (int8_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++)
	{
		if (_events[i].eventType != EVENT_NONE)
		{
			_events[i].update(now);
		}
	}
}
int8_t Timer::findFreeEventIndex(void)
{
	for (int8_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++)
	{
		if (_events[i].eventType == EVENT_NONE)
		{
			return i;
		}
	}
	return NO_TIMER_AVAILABLE;
}
