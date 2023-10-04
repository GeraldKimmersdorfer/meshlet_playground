#include "NoCompression.h"

//#include "../shared_data.h"

/*
#include "sequential_invoker.hpp"
#include "material_image_helpers.hpp"
#include "configure_and_compose.hpp"*/

void NoCompression::doCompress(avk::queue* queue)
{
	/*
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device,
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mShared->mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mShared->mVertexData.data(), 0) }, *queue)->wait_until_signalled();*/
	//mAdditionalDescriptorBindings.push_back(avk::descriptor_binding(3, 0, mShared->mVertexBuffer));
}

void NoCompression::doDestroy()
{
	//mVertexBuffer = avk::buffer();
}
