#include "Timer.h"

Timer::Timer()
{
	m_startTime = steady_clock::now();
	Tick();
}

double Timer::GetElapsedTime()
{
	return duration<double>(steady_clock::now() - m_startTime).count();
}

double Timer::GetDeltaTime()
{
	return duration<double>(steady_clock::now() - m_lastFrame).count();
}

void Timer::Tick()
{
	m_lastFrame = steady_clock::now();
}
