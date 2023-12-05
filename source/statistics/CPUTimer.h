#pragma once

#include "TimerInterface.h"
#include "chrono"

/// The CpuTimer class measures times on the c++ side using the std::chronos library
class CpuTimer : public TimerInterface {
public:
	CpuTimer(const std::string& name, const std::string& group, int queue_size, float average_weight);

protected:
	// starts front-buffer query
	void _start() override;
	// stops front-buffer query and toggles indices
	void _stop() override;
	// fetches back-buffer query
	float _fetch_result() override;

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_ticks[2];
};