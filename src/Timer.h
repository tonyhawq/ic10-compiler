#pragma once

#include <chrono>

class Timer
{
public:
	Timer();
	~Timer();

	double start();
	double time();
private:
	std::chrono::high_resolution_clock::time_point m_start_point;
};
