
#define ENTRY_COUNT 3

/* =======================================================================================
/*  NO COMPRESSION (NOCOMP)
/* ======================================================================================= */
#if MCC_VERTEX_COMPRESSION == _NOCOMP

layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data_no_compression vertices[]; };

vertex_data getVertexData(uint vid, uint mid) {
    vertex_data ret;
    ret.mPosition = vertices[vid].mPositionTxX.xyz;
    ret.mNormal = vertices[vid].mTxYNormal.yzw;
    ret.mTexCoord = vec2(vertices[vid].mPositionTxX.w, vertices[vid].mTxYNormal.x);
    ret.mBoneIndices = vertices[vid].mBoneIndices;
    ret.mBoneWeights = vertices[vid].mBoneWeights;
    return ret;
}

/* =======================================================================================
/*  LOOKUP TABLE COMPRESSION (LUT)
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _LUT

#extension EXT_shader_explicit_arithmetic_types : enable
layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_bone_lookup vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 bone_indices_lut[]; };

vertex_data getVertexData(uint vid, uint mid) {
    vertex_data ret;
    ret.mPosition = vertices[vid].mPositionTxX.xyz;
    ret.mNormal = vertices[vid].mTxYNormal.yzw;
    ret.mTexCoord = vec2(vertices[vid].mPositionTxX.w, vertices[vid].mTxYNormal.x);
    ret.mBoneIndices = uvec4(bone_indices_lut[vertices[vid].mBoneIndicesLUID]);
    ret.mBoneWeights = vec4(vertices[vid].mBoneWeights, 1.0);
    ret.mBoneWeights.w = 1.0 - ( ret.mBoneWeights.x + ret.mBoneWeights.y + ret.mBoneWeights.z );
    //ret.mBoneWeights = vertices[vid].mBoneWeightsGT;
    //ret.mBoneIndices = vertices[vid].mBoneIndicesGT;
    return ret;
}

/* =======================================================================================
/*  MESHLET RIGGED COMPRESSION (MLTR)
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _MLTR

layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_meshlet_coding vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 bone_indices_lut[]; };
layout(set = 3, binding = 2) buffer VertexPerMeshletBuffer { uint16_t meshlet_mbi_lut[]; };
#extension GL_EXT_control_flow_attributes : enable
#include "blend_attribute_compression.glsl"
#include "bit_coding.glsl"
#include "permutation.glsl"

vertex_data getVertexData(uint vid, uint mid) {
    vertex_data ret;
    ret.mPosition = decode_position_2x32(vertices[vid].mPosition);
    ret.mNormal = octahedronDecode(vec2(
        bitfieldExtract(vertices[vid].mNormal, 16, 16) / 65534.0,
        bitfieldExtract(vertices[vid].mNormal, 0, 16) / 65534.0
    ));

    ret.mTexCoord = vec2(
        bitfieldExtract(vertices[vid].mTexCoords, 16, 16) / 65534.0,
        bitfieldExtract(vertices[vid].mTexCoords, 0, 16) / 65534.0
    );

    uvec2 code = uvec2(vertices[vid].mWeightsImbiluidPermutation, 0);
    bool valid; // not really necessary...
    blend_attribute_codec_t codec;
    codec.weight_value_count = 18;
    codec.extra_value_counts[0] = 1;
    codec.extra_value_counts[1] = 1;
    codec.extra_value_counts[2] = 2;
    codec.payload_value_count_over_factorial = 0; // irrelevant (only for valid check)
    float out_weights[ENTRY_COUNT + 1];
    uint tuple_index = decompress_blend_attributes(out_weights, valid, code, codec);

    uint mbiluid = 0; uint permutation = 0;
    unpackMbiluidAndPermutation(tuple_index, mbiluid, permutation);

    uint luid = uint(meshlet_mbi_lut[mid * 4 + mbiluid]);

    ret.mBoneWeights = vec4(out_weights[3], out_weights[2], out_weights[1], out_weights[0]);
    ret.mBoneIndices = uvec4(bone_indices_lut[luid]);
    ret.mBoneIndices = applyPermutationInverse(ret.mBoneIndices, permutation);
    return ret;
}

/* =======================================================================================
/*  DYNAMIC MESHLET VERTEX CODING (DMLT)
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _DMLT
// NOT IMPLEMENTED

/* =======================================================================================
/*  PERMUTATION CODING CODEC (PC)
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _PC

layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_permutation_coding vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 bone_indices_lut[]; };
#extension GL_EXT_control_flow_attributes : enable
#include "blend_attribute_compression.glsl"
#include "bit_coding.glsl"

vertex_data getVertexData(uint vid, uint mid) {
    vertex_data ret;
    ret.mPosition = decode_position_2x32(vertices[vid].mPosition);
    ret.mNormal = octahedronDecode(vec2(
        bitfieldExtract(vertices[vid].mNormal, 16, 16) / 65534.0,
        bitfieldExtract(vertices[vid].mNormal, 0, 16) / 65534.0
    ));

    ret.mTexCoord = vec2(
        bitfieldExtract(vertices[vid].mTexCoords, 16, 16) / 65534.0,
        bitfieldExtract(vertices[vid].mTexCoords, 0, 16) / 65534.0
    );

    uvec2 code = uvec2(vertices[vid].mBoneData, 0);
    bool valid; // not really necessary...
    blend_attribute_codec_t codec;
    codec.weight_value_count = 18;
    codec.extra_value_counts[0] = 1;
    codec.extra_value_counts[1] = 1;
    codec.extra_value_counts[2] = 2;
    codec.payload_value_count_over_factorial = 0; // irrelevant (only for valid check)
    float out_weights[ENTRY_COUNT + 1];
    uint tuple_index = decompress_blend_attributes(out_weights, valid, code, codec);

    ret.mBoneWeights = vec4(out_weights[3], out_weights[2], out_weights[1], out_weights[0]);
    ret.mBoneIndices = uvec4(bone_indices_lut[tuple_index]);
    return ret;
}


/* ====================================EOF================================================ */
#endif

