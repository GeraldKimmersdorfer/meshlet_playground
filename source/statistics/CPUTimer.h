/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "TimerInterface.h"
#include "chrono"

 /// The CpuTimer class measures times on the c++ side using the std::chronos library
class CpuTimer : public TimerInterface {
public:
	CpuTimer(std::shared_ptr<PropertyInterface> property);

	void start();

	void stop();

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_ticks[2];
};