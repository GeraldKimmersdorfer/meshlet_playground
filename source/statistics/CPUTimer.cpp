#include "CpuTimer.h"

CpuTimer::CpuTimer(std::shared_ptr<PropertyInterface> property)
	:TimerInterface(std::move(property))
{}

void CpuTimer::start()
{
	m_ticks[0] = std::chrono::high_resolution_clock::now();
}

void CpuTimer::stop()
{
	m_ticks[1] = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = m_ticks[1] - m_ticks[0];
	m_prop->setFloat(diff.count() * 1000.0);
}
