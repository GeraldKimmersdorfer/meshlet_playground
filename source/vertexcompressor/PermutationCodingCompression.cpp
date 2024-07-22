#include "PermutationCodingCompression.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"

#include <glm/gtc/epsilon.hpp>

#define BIT_DEPTH_TESTS 1

void PermutationCodingCompression::doCompress(avk::queue* queue)
{
	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

#if BIT_DEPTH_TESTS
    std::vector<float> errors8Bit, errors16Bit, errors21Bit;
#endif

    mVertexData.reserve(mShared->mVertexData.size());
    for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
        auto& vert = mShared->mVertexData[vid];
        auto& newVert = mVertexData.emplace_back(vertex_data_permutation_coding{});

        const auto originalPosition = glm::vec3(vert.mPositionTxX.x, vert.mPositionTxX.y, vert.mPositionTxX.z);
        newVert.mPosition = encodeVec3ToUVec2(originalPosition);
        newVert.mNormal = packNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
        newVert.mTexCoords = packTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
        newVert.mBoneData = PermutationCodec::encode(vert.mBoneWeights, vertexLUIndexTable[vid]);

#if BIT_DEPTH_TESTS
        mesh_data* mesh = nullptr;
        for (int i = 0; i < mShared->mMeshData.size(); i++) {
            mesh = &mShared->mMeshData[i];
            if (vid >= mesh->mVertexOffset && vid < mesh->mVertexOffset + mesh->mVertexCount) {
                break;
            }
        }
        assert(mesh != nullptr);
		const glm::vec3 normalInvScale = glm::vec3(mesh->mPositionNormalizationInvScale);
		const glm::vec3 normalInvTranslation = glm::vec3(mesh->mPositionNormalizationInvTranslation);
        const auto originalPositionLocal = originalPosition * normalInvScale + normalInvTranslation;

        // 8-bit quantization
        auto quantized8Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 255.0f + 0.5f) / 255.0f * normalInvScale + normalInvTranslation;
        float error8Bit = glm::length(originalPositionLocal - quantized8Bit);
        errors8Bit.push_back(error8Bit);

        // 16-bit quantization
        auto quantized16Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 65535.0f + 0.5f) / 65535.0f * normalInvScale + normalInvTranslation;
        float error16Bit = glm::length(originalPositionLocal - quantized16Bit);
        errors16Bit.push_back(error16Bit);

        // 21-bit quantization
        auto quantized21Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 2097151.0f + 0.5f) / 2097151.0f * normalInvScale + normalInvTranslation;
        float error21Bit = glm::length(originalPositionLocal - quantized21Bit);
        errors21Bit.push_back(error21Bit);
#endif
    }

#if BIT_DEPTH_TESTS
    auto calculateMeanAndStdDev = [](const std::vector<float>& errors) {
        float sum = std::accumulate(errors.begin(), errors.end(), 0.0f);
        float mean = sum / errors.size();

        float accum = 0.0f;
        std::for_each(errors.begin(), errors.end(), [&](const float d) {
            accum += (d - mean) * (d - mean);
            });

        float stdev = std::sqrt(accum / (errors.size() - 1));
        return std::make_pair(mean, stdev);
        };

    auto [meanError8Bit, stdDev8Bit] = calculateMeanAndStdDev(errors8Bit);
    auto [meanError16Bit, stdDev16Bit] = calculateMeanAndStdDev(errors16Bit);
    auto [meanError21Bit, stdDev21Bit] = calculateMeanAndStdDev(errors21Bit);

    std::cout << "Vertex Count:" << mShared->mVertexData.size() << std::endl;

    std::cout << "8-bit Quantization - Mean Error: " << meanError8Bit << ", Std Dev: " << stdDev8Bit << std::endl;
    std::cout << "16-bit Quantization - Mean Error: " << meanError16Bit << ", Std Dev: " << stdDev16Bit << std::endl;
    std::cout << "21-bit Quantization - Mean Error: " << meanError21Bit << ", Std Dev: " << stdDev21Bit << std::endl;
#endif

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
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(vertex_data_permutation_coding) * mVertexData.size());
	mShared->mPropertyManager->get("amb_size")->setUint(0);
}

void PermutationCodingCompression::doDestroy()
{
	mVertexData.clear();
	mBoneLUTData.clear();
	mBoneLUTBuffer = avk::buffer();
	mVertexBuffer = avk::buffer();
}

void PermutationCodingCompression::hud_config(bool& config_has_changed)
{
	//ImGui::Checkbox("LUT with ID-Shuffle", &mWithShuffle);
}
