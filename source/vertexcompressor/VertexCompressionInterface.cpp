#include "VertexCompressionInterface.h"

void VertexCompressionInterface::compress(avk::queue* queue)
{
    mAdditionalDescriptorBindings.clear();
    doCompress(queue);
}

void VertexCompressionInterface::destroy()
{
    doDestroy();
    mAdditionalDescriptorBindings.clear();
}

std::vector<avk::binding_data> VertexCompressionInterface::getBindings()
{
    return mAdditionalDescriptorBindings;
}
