/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

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