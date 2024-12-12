#pragma once
#include <chrono>

using namespace std::chrono;

class Timer
{
public:
	Timer();

	double GetElapsedTime();
	double GetDeltaTime();
	void Tick();

private:
	steady_clock::time_point startTime;
	steady_clock::time_point lastFrame;
};