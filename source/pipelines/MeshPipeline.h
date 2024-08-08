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

#pragma once

#include "PipelineInterface.h"
#include "mcc.h"

class SharedData;

class MeshPipeline : public PipelineInterface {

public:

	MeshPipeline(SharedData* shared);

	avk::command::action_type_command render(int64_t inFlightIndex) override;

	void hud_config(bool& config_has_changed) override;

	void hud_setup(bool& config_has_changes) override;

	void compile() override;

private:
	uint32_t mTaskInvocations;
	uint32_t mMeshInvocations;

	avk::buffer mMeshletsBuffer;
	avk::buffer mPackedIndexBuffer;

	std::vector<avk::binding_data> mAdditionalStaticDescriptorBindings;

	std::pair<MCC_MESHLET_EXTENSION, MCC_MESHLET_EXTENSION> mMeshletExtension = { _NV, _NV };	// first ... avtive, second ... selected
	std::pair<MCC_MESHLET_TYPE, MCC_MESHLET_TYPE> mMeshletType = { _NATIVE, _NATIVE };			// first ... avtive, second ... selected

	std::string mPathTaskShader = "";
	std::string mPathMeshShader = "";
	std::string mPathFragmentShader = "";
	bool mShadersRecompiled = false;

	avk::graphics_pipeline mPipeline;

	void doInitialize(avk::queue* queue) override;

	void doDestroy() override;

};