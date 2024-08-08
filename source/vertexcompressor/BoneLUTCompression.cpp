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

#include "BoneLUTCompression.h"
#include "../helpers/lut.h"
#include <imgui.h>

void BoneLUTCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(mWithReuse, mWithShuffle, mWithMerge, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		glm::vec4 boneWeights = vert.mBoneWeights;
		if (mWithShuffle) boneWeights = applyPermutation(boneWeights, vertexLUPermutation[vid]);
		auto newVertexData = mVertexData.emplace_back(vertex_data_bone_lookup{
			.mPositionTxX = vert.mPositionTxX,
			.mTxYNormal = vert.mTxYNormal,
			.mBoneWeights = glm::vec3(boneWeights),
			.mBoneIndicesLUID = static_cast<uint32_t>(vertexLUIndexTable[vid])
			});
	}
	mVertexBuffer = avk::context().create_buffer(avk::memory_usage::device,
		VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eVertexBuffer,
		avk::storage_buffer_meta::create_from_data(mVertexData)
	);
	avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mVertexData.data(), 0) }, *queue)->wait_until_signalled();

	mBoneLUTBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
		avk::storage_buffer_meta::create_from_data(mBoneLUTData)
	);
	avk::context().record_and_submit_with_fence({ mBoneLUTBuffer->fill(mBoneLUTData.data(), 0) }, *queue)->wait_until_signalled();

	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 0, mVertexBuffer));
	mAdditionalStaticDescriptorBindings.push_back(avk::descriptor_binding(3, 1, mBoneLUTBuffer));

	// report to props:
	mShared->mPropertyManager->get("lut_size")->setUint(mBoneLUTData.size() * sizeof(glm::u16vec4));
	mShared->mPropertyManager->get("lut_count")->setUint(mBoneLUTData.size());
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(vertex_data_bone_lookup) * mVertexData.size());
	mShared->mPropertyManager->get("emb_size")->setUint(0);
}

void BoneLUTCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void BoneLUTCompression::hud_config(bool& config_has_changed)
{
	ImGui::Checkbox("LUT with Luid-Reuse", &mWithReuse);
	ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
	if (mWithShuffle) {
		ImGui::Checkbox("LUT with Merging", &mWithMerge);
	}
}
