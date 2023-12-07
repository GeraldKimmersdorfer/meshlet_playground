#pragma once

#include "TimerInterface.h"
#include "chrono"

#include "../SharedData.h"

class GpuTimer : public TimerInterface {
	static uint32_t gputimer_counter;
	static avk::query_pool gputimer_querypool;
	static uint32_t gputimer_frames_inflight;
	static double gputimer_timestamp_period;

public:
	GpuTimer(const std::string& name, const std::string& group, int queue_size, float average_weight);
	~GpuTimer();

	avk::command::action_type_command startAction(uint32_t inFlightIndex);

	avk::command::action_type_command stopAction(uint32_t inFlightIndex);

protected:

	void _start() override {};
	void _stop() override {};
	float _fetch_result() override;

private:
	int m_gputimer_id = 0;
	std::vector<bool> m_ready_to_fetch;
};