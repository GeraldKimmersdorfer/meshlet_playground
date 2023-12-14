#pragma once

#include "TimerInterface.h"
#include "../SharedData.h"	// Include for all the avk stuff...

class AvkTimer : public TimerInterface {
	static uint32_t gputimer_counter;
	static avk::query_pool gputimer_querypool;
	static uint32_t gputimer_frames_inflight;
	static double gputimer_timestamp_period;

public:
	AvkTimer(std::shared_ptr<PropertyInterface> property);
	~AvkTimer();

	avk::command::action_type_command start(uint32_t inFlightIndex);

	avk::command::action_type_command stop(uint32_t inFlightIndex);

private:
	int m_gputimer_id = 0;
	std::vector<bool> m_ready_to_fetch;
};