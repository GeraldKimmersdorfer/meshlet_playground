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