#pragma once

#include "../SharedData.h"

/// <summary>
/// This is the abstract interface of a pipeline object.
/// </summary>
class PipelineInterface {

public:

	PipelineInterface(SharedData* shared): 
		mShared(shared) 
	{};

	/// <summary>
	/// Creates CPU/GPU Buffers, compresses vertex attributes,...
	/// </summary>
	void initialize(avk::queue* queue) {
		mDescriptorCache = avk::context().create_descriptor_cache();
		doInitialize(queue);
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
		mDescriptorCache = avk::descriptor_cache();
	}

	/// <summary>
	/// Allows for custom ImGUI elements, which should be used for specific configurations
	/// of the pipeline. 
	/// </summary>
	virtual void hud() {};

protected:

	std::string mName;
	avk::descriptor_cache mDescriptorCache;
	SharedData* mShared;

private:
	virtual void doInitialize(avk::queue* queue) = 0;
	virtual void doDestroy() = 0;

};