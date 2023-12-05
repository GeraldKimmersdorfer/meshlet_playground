
#include "TimerInterface.h"

TimerInterface::TimerInterface(const std::string& name, const std::string& group, int queue_size, float average_weight)
	:m_name(name), m_group(group), m_queue_size(queue_size), m_average_weight(average_weight)
{
}

void TimerInterface::start() {
	//assert(m_state == TimerStates::READY);
	m_state = TimerStates::RUNNING;
	_start();
}

void TimerInterface::stop() {
	//assert(m_state == TimerStates::RUNNING);
	m_state = TimerStates::STOPPED;
	_stop();
}

bool TimerInterface::fetch_result() {
	if (m_state == TimerStates::STOPPED) {
		float val = _fetch_result();
		add_result(val);
		m_state = TimerStates::READY;
		return true;
	}
	return false;
}

void TimerInterface::add_result(float measurement)
{
	this->m_last_value = measurement;
	if (m_averaged_value < 0.0f) m_averaged_value = measurement;
	else m_averaged_value = (1.0f - m_average_weight) * m_averaged_value + m_average_weight * measurement;
}

