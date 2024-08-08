/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "NoCompression.h"

void NoCompression::doCompress(avk::queue* queue)
{

	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device,
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mShared->mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mShared->mVertexData.data(), 0) }, *queue)->wait_until_signalled();
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 0, mShared->mVertexBuffer));

	// report to props:
	mShared->mPropertyManager->get("lut_size")->setUint(0);
	mShared->mPropertyManager->get("lut_count")->setUint(0);
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(vertex_data) * mShared->mVertexData.size());
	mShared->mPropertyManager->get("emb_size")->setUint(0);
}

void NoCompression::doDestroy()
{
	mVertexBuffer = avk::buffer();
}
