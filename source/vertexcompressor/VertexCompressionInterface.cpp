#include "VertexCompressionInterface.h"

void VertexCompressionInterface::compress(avk::queue* queue)
{
	LOG_S(INFO) << "Vertex Compressor " << mName << ": Compressing " << mShared->mVertexData.size() << " vertices";
	mAdditionalStaticDescriptorBindings.clear();
	doCompress(queue);
}

void VertexCompressionInterface::destroy()
{
	doDestroy();
	mAdditionalStaticDescriptorBindings.clear();
	LOG_S(INFO) << "Vertex Compressor " << mName << " destroyed";
}


std::vector<avk::binding_data> VertexCompressionInterface::getBindings()
{
	return mAdditionalStaticDescriptorBindings;
}
