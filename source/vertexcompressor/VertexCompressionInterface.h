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