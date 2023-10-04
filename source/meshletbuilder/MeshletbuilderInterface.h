#pragma once

#include <string>
#include <vector>
#include "../shared_structs.h"

class SharedData;

class MeshletbuilderInterface {

public:

	MeshletbuilderInterface(const std::string& name, SharedData* shared) :
		mName(name), mShared(shared)
	{};

	void generate();
	
	void destroy();

	const std::string& getName() { return mName; }
	const std::pair<std::vector<meshlet_redirect>, std::vector<uint32_t>> getMeshletsRedirect();
	const std::vector<meshlet_native>& getMeshletsNative();

protected:

	virtual void doGenerate() = 0;
	virtual void doDestroy() {}

	std::vector<meshlet_native> mMeshletsNative;
	std::vector<meshlet_redirect> mMeshletsRedirect;
	std::vector<uint32_t> mRedirectPackedIndexData;

	std::string mName;

	SharedData* mShared;

	void generateRedirectedMeshletsFromNative();

private:
	// Buffer-Variable such that we can check when a model has been reloaded and meshlets need to be recreated
	uint32_t mGeneratedOnIndexCount = 0;
};