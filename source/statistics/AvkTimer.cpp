#include "AvkTimer.h"

#include <iostream>

uint32_t AvkTimer::gputimer_counter = 0;
avk::query_pool AvkTimer::gputimer_querypool = avk::query_pool();
uint32_t AvkTimer::gputimer_frames_inflight = 1;
double AvkTimer::gputimer_timestamp_period = 1.0;

AvkTimer::AvkTimer(std::shared_ptr<PropertyInterface> property)
	:TimerInterface(std::move(property))
{
	m_gputimer_id = gputimer_counter;
	gputimer_counter++;

	gputimer_frames_inflight = static_cast<uint32_t>(avk::context().main_window()->number_of_frames_in_flight());
	auto props = avk::context().physical_device().getProperties();
	gputimer_timestamp_period = static_cast<double>(props.limits.timestampPeriod);

	gputimer_querypool = avk::context().create_query_pool_for_timestamp_queries(
		gputimer_frames_inflight * 2 * gputimer_counter
	);
	m_ready_to_fetch.resize(gputimer_frames_inflight, false);

}

AvkTimer::~AvkTimer()
{
	gputimer_counter--;
	if (gputimer_counter == 0) {
		gputimer_querypool = avk::query_pool(); // Free ressource
	}
}

avk::command::action_type_command AvkTimer::start(uint32_t inFlightIndex)
{
	// First fetch the old results:
	auto firstPoolIndex = m_gputimer_id * (gputimer_frames_inflight * 2) + inFlightIndex * 2;
	if (m_ready_to_fetch[inFlightIndex]) // otherwise we will wait forever
	{
		auto timers = gputimer_querypool->get_results<uint64_t, 2>(
			firstPoolIndex, 2, vk::QueryResultFlagBits::e64 // | vk::QueryResultFlagBits::eWait // => ensure that the results are available (shouldnt be necessary)
			);
		m_prop->setFloat((double)(timers[1] - timers[0]) * 1e-6 * gputimer_timestamp_period);
	}
	// Now record to overwrite with new ones:
	m_ready_to_fetch[inFlightIndex] = true;
	return avk::command::custom_commands([this, firstPoolIndex](avk::command_buffer_t& cb) {
		cb.record({
			gputimer_querypool->reset(firstPoolIndex, 2),
			gputimer_querypool->write_timestamp(firstPoolIndex + 0, avk::stage::all_commands)
			});
		});
}

avk::command::action_type_command AvkTimer::stop(uint32_t inFlightIndex)
{
	auto firstPoolIndex = m_gputimer_id * (gputimer_frames_inflight * 2) + inFlightIndex * 2;
	return gputimer_querypool->write_timestamp(firstPoolIndex + 1, avk::stage::all_commands);
}