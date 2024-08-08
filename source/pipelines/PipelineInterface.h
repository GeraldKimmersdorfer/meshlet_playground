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

#include "../SharedData.h"

 /// <summary>
 /// This is the abstract interface of a pipeline object.
 /// </summary>
class PipelineInterface {

public:

	PipelineInterface(SharedData* shared, const std::string& name) :
		mShared(shared), mName(name)
	{};

	/// <summary>
	/// Creates CPU/GPU Buffers, compresses vertex attributes,...
	/// </summary>
	void initialize(avk::queue* queue) {
		LOG_S(INFO) << "Initializing pipeline " << mName;
		doInitialize(queue);
		initCount++;
	}

	/// <summary>
	/// Renders the data
	/// </summary>
	virtual avk::command::action_type_command render(int64_t inFlightIndex) = 0;

	/// <summary>
	/// Frees all the CPU/GPU related data, such that for complex scenes I don't run into
	/// memory issues. Settings should not be destroyed upon destroy()
	/// </summary>
	void destroy() {
		doDestroy();
		LOG_S(INFO) << "Pipeline " << mName << " destroyed";
	}

	/// <summary>
	/// Additional step that gets executed before initializing. If necessary this step
	/// is meant to compile a shader. Any exceptions will be caught and displayed in the GUI.
	/// </summary>
	virtual void compile() {};

	/// <summary>
	/// Allows for custom ImGUI elements, which should be used for specific configurations
	/// of the pipeline. 
	/// </summary>
	/// <param name="config_has_changed">set to true if config ubo is supposed to be updated</param>
	virtual void hud_config(bool& config_has_changed) {};

	/// <summary>
	/// Allows for custom ImGUI elements BEFORE the pipeline is loaded. Should be used for precompile variables
	/// </summary>
	/// <param name="config_has_changed">set to true if config ubo is supposed to be updated</param>
	virtual void hud_setup(bool& config_has_changed) {};

	const std::string& getName() { return mName; }

protected:

	std::string mName;
	SharedData* mShared;
	int initCount = 0;

private:
	virtual void doInitialize(avk::queue* queue) = 0;
	virtual void doDestroy() = 0;

};