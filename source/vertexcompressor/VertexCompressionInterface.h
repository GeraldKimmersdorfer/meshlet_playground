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

#include <string>
#include <vector>
#include "../SharedData.h"


class VertexCompressionInterface {
public:

	VertexCompressionInterface(SharedData* shared, const std::string& name, const std::string& mccId)
		: mShared(shared), mName(name), mMccId(mccId)
	{}

	void compress(avk::queue* queue);

	void destroy();

	virtual void hud_config(bool& config_has_changed) {}

	std::vector<avk::binding_data> getBindings();

	const std::string& getName() { return mName; }

	const std::string& getMccId() { return mMccId; }

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) = 0;

	// Has to free all ressources
	virtual void doDestroy() = 0;

	std::string mName;
	std::string mMccId;	// e.g. _NOCOMP
	SharedData* mShared;
	std::vector<avk::binding_data> mAdditionalStaticDescriptorBindings;

private:

};