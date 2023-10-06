#include "VertexCompressionInterface.h"

void VertexCompressionInterface::compress(avk::queue* queue)
{
    mAdditionalStaticDescriptorBindings.clear();
    doCompress(queue);
}

void VertexCompressionInterface::destroy()
{
    doDestroy();
    mAdditionalStaticDescriptorBindings.clear();
}


std::vector<avk::binding_data> VertexCompressionInterface::getBindings()
{
    return mAdditionalStaticDescriptorBindings;
}
