/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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