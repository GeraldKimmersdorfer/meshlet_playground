#pragma once

#include <string>
#include <avk/avk.hpp>

class SharedData;

class VertexCompressionInterface {
public:

	VertexCompressionInterface(const std::string& name, SharedData* shared)
		: mShared(shared), mName(name)
	{}

	void compress(avk::queue* queue);

	void destroy();

	std::vector<avk::binding_data> getBindings();

	const std::string& getName() { return mName; }

protected:

	// Has to build all the buffers
	virtual void doCompress(avk::queue* queue) = 0;

	// Has to free all ressources
	virtual void doDestroy() = 0;

	std::string mName;
	SharedData* mShared;
	std::vector<avk::binding_data> mAdditionalDescriptorBindings;

private:

};