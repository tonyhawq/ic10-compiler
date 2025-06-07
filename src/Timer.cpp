#include "Timer.h"

Timer::Timer()
	:m_start_point(std::chrono::high_resolution_clock::now())
{}

Timer::~Timer()
{}

double Timer::start()
{
	double value = time();
	this->m_start_point = std::chrono::high_resolution_clock::now();
	return value;
}

double Timer::time()
{
	std::chrono::duration<double, std::ratio<1,1>> duration = std::chrono::high_resolution_clock::now() - this->m_start_point;
	return duration.count();
}
