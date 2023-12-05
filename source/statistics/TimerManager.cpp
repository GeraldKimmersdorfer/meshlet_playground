#include "TimerManager.h"


TimerManager::TimerManager()
{
}

void TimerManager::start_timer(const std::string& name)
{
	auto it = m_timer.find(name);
	if (it != m_timer.end()) m_timer[name]->start();
}

void TimerManager::stop_timer(const std::string& name)
{
	auto it = m_timer.find(name);
	if (it != m_timer.end()) m_timer[name]->stop();
}

std::shared_ptr<TimerInterface> TimerManager::get(const std::string& name)
{
	auto it = m_timer.find(name);
	if (it != m_timer.end()) return it->second;
	return nullptr;
}

void TimerManager::fetch_results()
{
	for (const auto& tmr : m_timer_in_order) {
		tmr->fetch_result();
	}
}


std::shared_ptr<TimerInterface> TimerManager::add_timer(std::shared_ptr<TimerInterface> tmr) {
	m_timer[tmr->get_name()] = tmr;
	auto tmr_group = tmr->get_group();
	// NOTE: The following code makes sure that the timers of the different groups are next to each other inside
	// the m_timer_in_order vector. Inside their group they are sorted by the order of when they are added.
	std::string last_group;
	bool inserted = false;
	for (int i = 0; i < m_timer_in_order.size(); i++) {
		auto next_group = m_timer_in_order[i]->get_group();
		if (last_group == tmr_group && next_group != tmr_group) {
			// meaning i points to first element of the next group, so we want to insert it before i
			m_timer_in_order.insert(m_timer_in_order.begin() + i, tmr);
			inserted = true;
			break;
		}
		last_group = next_group;
	}
	if (!inserted) m_timer_in_order.push_back(tmr);
	return tmr;
}