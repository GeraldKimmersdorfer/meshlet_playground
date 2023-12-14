#pragma once

#include <memory>
#include "PropertyInterface.h"


class TimerInterface {

public:
	TimerInterface(std::shared_ptr<PropertyInterface> property)
		: m_prop(property)
	{}

protected:
	std::shared_ptr<PropertyInterface> m_prop;
};