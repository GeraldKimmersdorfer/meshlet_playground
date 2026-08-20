/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "PermutationCodingCompression.h"
#include "../helpers/lut.h"
#include <imgui.h>
#include "../helpers/packing.h"
#include "../helpers/permcodec.h"

#include <glm/gtc/epsilon.hpp>

#define BIT_DEPTH_TESTS 0

#if BIT_DEPTH_TESTS
bool areEqual(glm::vec3 a, glm::vec3 b, float epsilon = 1e-5f)
{
	return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(epsilon)));
}
#endif

void PermutationCodingCompression::doCompress(avk::queue* queue)
{

	std::vector<uint16_t> vertexLUIndexTable; std::vector<uint8_t> vertexLUPermutation;
	createBoneIndexLUT(true, false, false, mShared->mVertexData, mBoneLUTData, &vertexLUIndexTable, &vertexLUPermutation);

#if BIT_DEPTH_TESTS
	std::vector<float> errors8Bit, errors10bit, errors16Bit, errors21Bit;
	std::vector<float> normErrors8Bit, normErrors10bit, normErrors16Bit, normErrors21Bit;
	std::vector<float> errorsNormals16BitOct, errorsNormals16BitSph, errorsNormals16BitEucl;
#endif

	mVertexData.reserve(mShared->mVertexData.size());
	for (uint32_t vid = 0; vid < mShared->mVertexData.size(); vid++) {
		auto& vert = mShared->mVertexData[vid];
		auto& newVert = mVertexData.emplace_back(vertex_data_permutation_coding{});

		const auto originalPosition = glm::vec3(vert.mPositionTxX.x, vert.mPositionTxX.y, vert.mPositionTxX.z);
		newVert.mPosition = encodeVec3ToUVec2(originalPosition);
		newVert.mNormal = packNormal(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));
		newVert.mTexCoords = packTextureCoords(glm::vec2(vert.mPositionTxX.w, vert.mTxYNormal.x));
		newVert.mBoneData = PermutationCodec::encode(vert.mBoneWeights, vertexLUIndexTable[vid], PermutationCodec::codec16bit);

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
		const float volume = glm::length(normalInvScale);
		const glm::vec3 normalInvTranslation = glm::vec3(mesh->mPositionNormalizationInvTranslation);
		const auto originalPositionLocal = originalPosition * normalInvScale + normalInvTranslation;

		// 8-bit quantization
		auto quantized8Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 255.0f + 0.5f) / 255.0f * normalInvScale + normalInvTranslation;
		float error8Bit = glm::length(originalPositionLocal - quantized8Bit) / volume;
		errors8Bit.push_back(error8Bit);

		auto quantized10Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 1023.0f + 0.5f) / 1023.0f * normalInvScale + normalInvTranslation;
		float error10Bit = glm::length(originalPositionLocal - quantized10Bit) / volume;
		errors10bit.push_back(error10Bit);

		// 16-bit quantization
		auto quantized16Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 65535.0f + 0.5f) / 65535.0f * normalInvScale + normalInvTranslation;
		float error16Bit = glm::length(originalPositionLocal - quantized16Bit) / volume;
		errors16Bit.push_back(error16Bit);

		// 21-bit quantization
		auto quantized21Bit = glm::floor((originalPositionLocal - normalInvTranslation) / normalInvScale * 2097151.0f + 0.5f) / 2097151.0f * normalInvScale + normalInvTranslation;
		float error21Bit = glm::length(originalPositionLocal - quantized21Bit) / volume;
		errors21Bit.push_back(error21Bit);

		normErrors8Bit.push_back(glm::length(originalPosition - glm::floor(originalPosition * 255.0f) / 255.0f));
		normErrors10bit.push_back(glm::length(originalPosition - glm::floor(originalPosition * 1023.0f) / 1023.0f));
		normErrors16Bit.push_back(glm::length(originalPosition - glm::floor(originalPosition * 65535.0f) / 65535.0f));
		normErrors21Bit.push_back(glm::length(originalPosition - glm::floor(originalPosition * 2097151.0f) / 2097151.0f));

		// Calculate errors for normal quantization using spherical encoding/decoding
		auto originalNormal = glm::normalize(glm::vec3(vert.mTxYNormal.y, vert.mTxYNormal.z, vert.mTxYNormal.w));

		if (glm::length(originalNormal) > 0.0001f) {
			// Eucledian encoding
			glm::vec3 euclidianQuantized16Bit = glm::floor((originalNormal * 0.5f + 0.5f) * 1023.0f + 0.5f) / 1023.0f;
			glm::vec3 euclidianDecoded = euclidianQuantized16Bit * 2.0f - 1.0f;
			float errorNormalEuclidian = glm::length(originalNormal - euclidianDecoded);
			errorsNormals16BitEucl.push_back(errorNormalEuclidian);

			// Spherical encoding
			glm::vec2 sphericalEncoded = sphericalEncode(originalNormal);
			glm::vec2 sphericalQuantized16Bit = glm::floor(sphericalEncoded * 65535.0f + 0.5f) / 65535.0f;
			glm::vec3 sphericalDecoded = sphericalDecode(sphericalQuantized16Bit);
			float errorNormalSpherical = glm::length(originalNormal - sphericalDecoded);
			errorsNormals16BitSph.push_back(errorNormalSpherical);

			// Octahedron encoding
			glm::vec2 octahedronEncoded = octahedronEncode(originalNormal);
			glm::vec2 octahedronQuantized16Bit = glm::floor(octahedronEncoded * 65535.0f + 0.5f) / 65535.0f;
			glm::vec3 octahedronDecoded = octahedronDecode(octahedronQuantized16Bit);
			float errorNormalOctahedron = glm::length(originalNormal - octahedronDecoded);
			errorsNormals16BitOct.push_back(errorNormalOctahedron);
		}

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
	auto [meanError10Bit, stdDev10Bit] = calculateMeanAndStdDev(errors10bit);
	auto [meanError16Bit, stdDev16Bit] = calculateMeanAndStdDev(errors16Bit);
	auto [meanError21Bit, stdDev21Bit] = calculateMeanAndStdDev(errors21Bit);

	auto [meanNormError8Bit, stdDevNormError8Bit] = calculateMeanAndStdDev(normErrors8Bit);
	auto [meanNormError10Bit, stdDevNormError10Bit] = calculateMeanAndStdDev(normErrors10bit);
	auto [meanNormError16Bit, stdDevNormError16Bit] = calculateMeanAndStdDev(normErrors16Bit);
	auto [meanNormError21Bit, stdDevNormError21Bit] = calculateMeanAndStdDev(normErrors21Bit);


	std::cout << "Vertex Count:" << mShared->mVertexData.size() << std::endl;

	std::cout << "8-bit Quantization - Mean Error: " << meanError8Bit << ", Std Dev: " << stdDev8Bit << std::endl;
	std::cout << "10-bit Quantization - Mean Error: " << meanError10Bit << ", Std Dev: " << stdDev10Bit << std::endl;
	std::cout << "16-bit Quantization - Mean Error: " << meanError16Bit << ", Std Dev: " << stdDev16Bit << std::endl;
	std::cout << "21-bit Quantization - Mean Error: " << meanError21Bit << ", Std Dev: " << stdDev21Bit << std::endl;

	std::cout << "== WITH NORMALIZED VALUES ==" << std::endl;
	std::cout << "8-bit Quantization - Mean Error: " << meanNormError8Bit << ", Std Dev: " << stdDevNormError8Bit << std::endl;
	std::cout << "10-bit Quantization - Mean Error: " << meanNormError10Bit << ", Std Dev: " << stdDevNormError10Bit << std::endl;
	std::cout << "16-bit Quantization - Mean Error: " << meanNormError16Bit << ", Std Dev: " << stdDevNormError16Bit << std::endl;
	std::cout << "21-bit Quantization - Mean Error: " << meanNormError21Bit << ", Std Dev: " << stdDevNormError21Bit << std::endl;

	auto [meanErrorNormalSph, stdDevNormalSph] = calculateMeanAndStdDev(errorsNormals16BitSph);
	auto [meanErrorNormalOct, stdDevNormalOct] = calculateMeanAndStdDev(errorsNormals16BitOct);
	auto [meanErrorNormalEucl, stdDevNormalEucl] = calculateMeanAndStdDev(errorsNormals16BitEucl);

	std::cout << "Normal Quantization (Euclidian) - Mean Error: " << meanErrorNormalEucl << ", Std Dev: " << stdDevNormalEucl << std::endl;
	std::cout << "Normal Quantization (Spherical) - Mean Error: " << meanErrorNormalSph << ", Std Dev: " << stdDevNormalSph << std::endl;
	std::cout << "Normal Quantization (Octahedron) - Mean Error: " << meanErrorNormalOct << ", Std Dev: " << stdDevNormalOct << std::endl;
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
	mShared->mPropertyManager->get("lut_count")->setUint(mBoneLUTData.size());
	mShared->mPropertyManager->get("lut_size")->setUint(mBoneLUTData.size() * sizeof(glm::u16vec4));
	mShared->mPropertyManager->get("vb_size")->setUint(sizeof(vertex_data_permutation_coding) * mVertexData.size());
	mShared->mPropertyManager->get("emb_size")->setUint(0);
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
