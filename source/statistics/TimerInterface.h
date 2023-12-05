#pragma once

#include <string>

enum class TimerStates { READY, RUNNING, STOPPED };

class TimerInterface {

public:

	TimerInterface(const std::string& name, const std::string& group, int queue_size, float average_weight);

	// Starts time-measurement
	void start();

	// Stops time-measurement
	void stop();

	// Fetches the result of the measuring and adds it to the average
	bool fetch_result();

	const std::string& get_name() { return this->m_name; }
	const std::string& get_group() { return this->m_group; }
	float get_last_value() { return this->m_last_value; }
	float get_averaged_value() { return this->m_averaged_value; }
	int get_queue_size() { return this->m_queue_size; }
	float get_average_weight() { return this->m_average_weight; }


protected:
	// a custom identifying name for this timer
	std::string m_name;
	std::string m_group;
	int m_queue_size;
	float m_average_weight;

	virtual void _start() = 0;
	virtual void _stop() = 0;
	virtual float _fetch_result() = 0;

	void add_result(float measurement);

private:

	TimerStates m_state = TimerStates::READY;
	float m_last_value = 0.0f;
	float m_averaged_value = -1.0f;
};