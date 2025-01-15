#include "Timer.h"

Timer::Timer()
{
	startTime = steady_clock::now();
	Tick();
}

double Timer::GetElapsedTime()
{
	return duration<double>(steady_clock::now() - startTime).count();
}

double Timer::GetDeltaTime()
{
	return duration<double>(steady_clock::now() - lastFrame).count();
}

void Timer::Tick()
{
	lastFrame = steady_clock::now();
}
