// The following variables are used for overlays and therefore have global scope
uint global_tuple_index = 0;
uint global_bone_attribute_resolution = 0;
uint global_bone_attribute_offset = 0;
uint global_bone_permutation = 0;

/* =======================================================================================
/*  NO COMPRESSION (NOCOMP)
/* ======================================================================================= */
#if MCC_VERTEX_COMPRESSION == _NOCOMP

layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data_no_compression vertices[]; };

vertex_data getVertexData(uint vid, uint mid) {
    vertex_data ret;
    //ret.mPosition = vertices[vid].mPositionTxX.xyz + vertices[vid].mTxYNormal.yzw * 0.00005 * mid;
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
    global_tuple_index = vertices[vid].mBoneIndicesLUID;
    ret.mBoneIndices = uvec4(bone_indices_lut[global_tuple_index]);
    ret.mBoneWeights = vec4(vertices[vid].mBoneWeights, 1.0);
    ret.mBoneWeights.w = 1.0 - ( ret.mBoneWeights.x + ret.mBoneWeights.y + ret.mBoneWeights.z );
    return ret;
}

/* =======================================================================================
/*  DUAL PERMUTATION DIFFERENCE CODEC (_DPDC16)
/* ======================================================================================= */

// TODO pack everything in boneAttributes32
//14 codec
//5 permut
//13 tuple
#elif MCC_VERTEX_COMPRESSION == _DPDC16

#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_control_flow_attributes : enable

#include "bit_coding.glsl"
#include "permutation_codec.glsl"
#include "permutation.glsl"

struct dmlt_meshlet_extension {
	uint boneAttributesMin;
	uint boneAttributesResolution; // 0, 1, 2, 3 (byte resolution necessary for this meshlet)
};

layout(set = 3, binding = 0, scalar) buffer VertexBuffer16bit { uint16_t vertices16bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };
layout(set = 3, binding = 2) buffer MeshletExtensionBuffer { dmlt_meshlet_extension meshletExtension[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id16 = vid / 2; // points to position

    uvec3 position16 = uvec3(uint(vertices16bit[id16 + 0]), uint(vertices16bit[id16 + 1]), uint(vertices16bit[id16 + 2]));
    uvec2 normal16 = uvec2(uint(vertices16bit[id16 + 3]), uint(vertices16bit[id16 + 4]));
    uvec2 texcoord16 = uvec2(uint(vertices16bit[id16 + 5]), uint(vertices16bit[id16 + 6]));
    
    // Fetch boneAttributes
    uint boneAttributes32 = uint(meshletExtension[mid].boneAttributesMin);
    {
        global_bone_attribute_resolution = uint(meshletExtension[mid].boneAttributesResolution);
        if (global_bone_attribute_resolution > 0) {
            if (global_bone_attribute_resolution == 2) {
                global_bone_attribute_offset = uint(vertices16bit[id16 + 7]);
            } else {
                global_bone_attribute_offset = concatenate2x16(uint(vertices16bit[id16 + 7]), uint(vertices16bit[id16 + 8]));
            }
            boneAttributes32 += global_bone_attribute_offset;
        }
    }

    uint payload; // permut and tuple index
    vec4 weights = decompress_pc_14x18(boneAttributes32, payload);
    global_tuple_index = payload & 0x1FFF; //bitfieldExtract(payload, 0, 13);
    global_bone_permutation = bitfieldExtract(payload, 13, 5);

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = weights;
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
    ret.mBoneIndices = applyPermutationInverse(ret.mBoneIndices, global_bone_permutation);
    return ret;
}

/* =======================================================================================
/* Permutation Difference Codec 8-bit padding [no permutation]
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _PDC8

#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_control_flow_attributes : enable

#include "bit_coding.glsl"
#include "permutation_codec.glsl"

struct difference_meshlet_extension {
	uint boneAttributesMin;
	uint boneAttributesResolution; // 0, 1, 2, 3 (byte resolution necessary for this meshlet)
};

layout(set = 3, binding = 0, scalar) buffer VertexBuffer8bit { uint8_t vertices8bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };
layout(set = 3, binding = 2) buffer MeshletExtensionBuffer { difference_meshlet_extension meshletExtension[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id8 = vid;

    uvec3 position16 = uvec3(
        concatenate2x8(uint(vertices8bit[id8 + 0]), uint(vertices8bit[id8 + 1])),
        concatenate2x8(uint(vertices8bit[id8 + 2]), uint(vertices8bit[id8 + 3])),
        concatenate2x8(uint(vertices8bit[id8 + 4]), uint(vertices8bit[id8 + 5]))
    );
    uvec2 normal16 = uvec2(
        concatenate2x8(uint(vertices8bit[id8 + 6]), uint(vertices8bit[id8 + 7])),
        concatenate2x8(uint(vertices8bit[id8 + 8]), uint(vertices8bit[id8 + 9]))
    );
    uvec2 texcoord16 = uvec2(
        concatenate2x8(uint(vertices8bit[id8 + 10]), uint(vertices8bit[id8 + 11])),
        concatenate2x8(uint(vertices8bit[id8 + 12]), uint(vertices8bit[id8 + 13]))
    );
    
    // Fetch boneAttributes
    uint boneAttributes32 = uint(meshletExtension[mid].boneAttributesMin);
    {
        global_bone_attribute_resolution = uint(meshletExtension[mid].boneAttributesResolution);
        switch(global_bone_attribute_resolution) {
            case 1: global_bone_attribute_offset = uint(vertices8bit[id8 + 14]); break;
            case 2: global_bone_attribute_offset = concatenate2x8(uint(vertices8bit[id8 + 14]), uint(vertices8bit[id8 + 15])); break;
            case 3: global_bone_attribute_offset = concatenate3x8(uint(vertices8bit[id8 + 14]), uint(vertices8bit[id8 + 15]), uint(vertices8bit[id8 + 16])); break;
            case 4: global_bone_attribute_offset = concatenate4x8(uint(vertices8bit[id8 + 14]), uint(vertices8bit[id8 + 15]), uint(vertices8bit[id8 + 16]), uint(vertices8bit[id8 + 17])); break;
        }
        boneAttributes32 += global_bone_attribute_offset;
    }

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = decompress_pc_16x16(boneAttributes32, global_tuple_index);
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
    return ret;
}

/* =======================================================================================
/* Permutation Difference Codec 16-bit padding [no permutation]
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _PDC16

#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_control_flow_attributes : enable

#include "bit_coding.glsl"
#include "permutation_codec.glsl"

struct dmlt_meshlet_extension {
	uint boneAttributesMin;
	uint boneAttributesResolution; // 0, 1, 2, 3 (byte resolution necessary for this meshlet)
};

layout(set = 3, binding = 0, scalar) buffer VertexBuffer16bit { uint16_t vertices16bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };
layout(set = 3, binding = 2) buffer MeshletExtensionBuffer { dmlt_meshlet_extension meshletExtension[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id16 = vid / 2; // points to position

    uvec3 position16 = uvec3(uint(vertices16bit[id16 + 0]), uint(vertices16bit[id16 + 1]), uint(vertices16bit[id16 + 2]));
    uvec2 normal16 = uvec2(uint(vertices16bit[id16 + 3]), uint(vertices16bit[id16 + 4]));
    uvec2 texcoord16 = uvec2(uint(vertices16bit[id16 + 5]), uint(vertices16bit[id16 + 6]));
    
    // Fetch boneAttributes
    uint boneAttributes32 = uint(meshletExtension[mid].boneAttributesMin);
    {
        global_bone_attribute_resolution = uint(meshletExtension[mid].boneAttributesResolution);
        if (global_bone_attribute_resolution > 0) {
            if (global_bone_attribute_resolution == 2) {
                global_bone_attribute_offset = uint(vertices16bit[id16 + 7]);
            } else {
                global_bone_attribute_offset = concatenate2x16(uint(vertices16bit[id16 + 7]), uint(vertices16bit[id16 + 8]));
            }
            boneAttributes32 += global_bone_attribute_offset;
        }
    }

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = decompress_pc_16x16(boneAttributes32, global_tuple_index);
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
#ifdef TMP
global_tmp_weights = ret.mBoneWeights;
    ret.mBoneWeights = vec4(1.0,0.0,0.0,0.0);
    ret.mBoneIndices = uvec4(1,0,0,0);
#endif
    return ret;
}

/* =======================================================================================
/* Permutation Difference Codec 32-bit padding [no permutation]
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _PDC32

#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_control_flow_attributes : enable

#include "bit_coding.glsl"
#include "permutation_codec.glsl"

struct dmlt_meshlet_extension {
    uint16_t tupleIndex; // MAX if tuple by vertex
	uint16_t w2;
	uint16_t w3;
	uint16_t w4;
};

layout(set = 3, binding = 0, scalar) buffer VertexBuffer16bit { uint16_t vertices16bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };
layout(set = 3, binding = 2, scalar) buffer MeshletExtensionBuffer { dmlt_meshlet_extension meshletExtension[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id16 = vid / 2; // points to position

    uvec3 position16 = uvec3(uint(vertices16bit[id16 + 0]), uint(vertices16bit[id16 + 1]), uint(vertices16bit[id16 + 2]));
    uvec2 normal16 = uvec2(uint(vertices16bit[id16 + 3]), uint(vertices16bit[id16 + 4]));
    uvec2 texcoord16 = uvec2(uint(vertices16bit[id16 + 5]), uint(vertices16bit[id16 + 6]));
    
    vec4 boneWeights = vec4(1.0, 0.0, 0.0, 0.0);
    global_tuple_index = uint(meshletExtension[mid].tupleIndex);
    if (global_tuple_index == 0xFFFF) {
        // use the value provided in the vertex (permutation encoded)
        global_bone_attribute_resolution = 4;
        uint boneAttributes32 = concatenate2x16(uint(vertices16bit[id16 + 7]), uint(vertices16bit[id16 + 8]));
        boneWeights = decompress_pc_16x16(boneAttributes32, global_tuple_index);
    } else {
        // use the value provided in the meshlet
        global_bone_attribute_resolution = 0;
        boneWeights.y = float(uint(meshletExtension[mid].w2)) / 65535.0 / 2.0;
        boneWeights.z = float(uint(meshletExtension[mid].w3)) / 65535.0 / 3.0;
        boneWeights.w = float(uint(meshletExtension[mid].w4)) / 65535.0 / 4.0;
        boneWeights.x = 1.0 - (boneWeights.y + boneWeights.z + boneWeights.w);
    }

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = boneWeights;
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
    return ret;
}

/* =======================================================================================
/*  PERMUTATION CODING CODEC (PC)
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _PC

layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_permutation_coding vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 bone_indices_lut[]; };
#extension GL_EXT_control_flow_attributes : enable
#include "permutation_codec.glsl"
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
    ret.mBoneWeights = decompress_pc_16x16(vertices[vid].mBoneData, global_tuple_index);
    ret.mBoneIndices = uvec4(bone_indices_lut[global_tuple_index]);
    return ret;
}

/* =======================================================================================
/*  QUICK PERMUTATION CODING
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _QPC

#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_control_flow_attributes : enable

#include "permutation_codec.glsl"
#include "bit_coding.glsl"
#include "permutation.glsl"

layout(set = 3, binding = 0) buffer VertexBuffer16bit { uint16_t vertices16bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id16 = vid * 18 / 2; // points to position

    uvec3 position16 = uvec3(uint(vertices16bit[id16 + 0]), uint(vertices16bit[id16 + 1]), uint(vertices16bit[id16 + 2]));
    uvec2 normal16 = uvec2(uint(vertices16bit[id16 + 3]), uint(vertices16bit[id16 + 4]));
    uvec2 texcoord16 = uvec2(uint(vertices16bit[id16 + 5]), uint(vertices16bit[id16 + 6]));
    uint boneAttributes32 = concatenate2x16(uint(vertices16bit[id16 + 7]), uint(vertices16bit[id16 + 8]));

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = decompress_pc_16x16(boneAttributes32, global_tuple_index);
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
    return ret;
}

/* =======================================================================================
/*  OPTIMAL SIMPLEX CODING
/* ======================================================================================= */
#elif MCC_VERTEX_COMPRESSION == _OSS

#extension GL_EXT_shader_16bit_storage   : require
#extension GL_EXT_control_flow_attributes : enable

#include "32OSS_ITS.fnc.glsl"
#include "bit_coding.glsl"

layout(set = 3, binding = 0) buffer VertexBuffer16bit { uint16_t vertices16bit[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { u16vec4 boneIndices[]; };

vertex_data getVertexData(uint vid, uint mid) {
    uint id16 = vid * 18 / 2; // points to position

    uvec3 position16 = uvec3(uint(vertices16bit[id16 + 0]), uint(vertices16bit[id16 + 1]), uint(vertices16bit[id16 + 2]));
    uvec2 normal16 = uvec2(uint(vertices16bit[id16 + 3]), uint(vertices16bit[id16 + 4]));
    uvec2 texcoord16 = uvec2(uint(vertices16bit[id16 + 5]), uint(vertices16bit[id16 + 6]));
    uint tupleIndex16 = uint(vertices16bit[id16 + 7]);
    uint boneWeights16 = uint(vertices16bit[id16 + 8]);

    vbac_ssq_fix4_info info = vbac_ssq_fix4_info(104,65231,0.0048543689320388345);
    vec4 weights = decompressWeights(boneWeights16, info);
    global_tuple_index = tupleIndex16;

    vertex_data ret;
    ret.mPosition = decode16(position16);
    ret.mNormal = octahedronDecode(decode16(normal16));
    ret.mTexCoord = decode16(texcoord16);
    ret.mBoneWeights = weights;
    ret.mBoneIndices = uvec4(boneIndices[global_tuple_index]);
    return ret;
}

/* ====================================EOF================================================ */
#endif

