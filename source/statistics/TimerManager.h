#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

#include "TimerInterface.h"
#include <memory>

class TimerManager
{

public:
	// adds the given timer
	std::shared_ptr<TimerInterface> add_timer(std::shared_ptr<TimerInterface> tmr);

	// Start timer with given name
	void start_timer(const std::string& name);

	// Stops the currently running timer
	void stop_timer(const std::string& name);

	std::shared_ptr<TimerInterface> get(const std::string& name);

	void fetch_results();

	const std::vector<std::shared_ptr<TimerInterface>>& get_timers() { return this->m_timer_in_order; }

	TimerManager();

private:
	// Contains the timer as list for the correct order
	std::vector<std::shared_ptr<TimerInterface>> m_timer_in_order;
	// Contains the timer as map for fast access by name
	std::map<std::string, std::shared_ptr<TimerInterface>> m_timer;

};