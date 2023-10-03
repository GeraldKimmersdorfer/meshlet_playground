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

	virtual void generate(uint32_t aMaxVertices, uint32_t aMaxIndices) = 0;

	const std::string& getName() { return mName; }
	const std::pair<std::vector<meshlet_redirect>, std::vector<uint32_t>> getMeshletsRedirect();
	const std::vector<meshlet_native>& getMeshletsNative();

protected:

	std::vector<meshlet_native> mMeshletsNative;
	std::vector<meshlet_redirect> mMeshletsRedirect;
	std::vector<uint32_t> mRedirectPackedIndexData;

	std::string mName;

	SharedData* mShared;

	void generateRedirectedMeshletsFromNative();

private:

};