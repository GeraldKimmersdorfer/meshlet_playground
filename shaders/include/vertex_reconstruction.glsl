
#define ENTRY_COUNT 3

#if MCC_VERTEX_COMPRESSION == _NOCOMP
layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data_no_compression vertices[]; };
#elif MCC_VERTEX_COMPRESSION == _LUT
layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_bone_lookup vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { uvec4 bone_indices_lut[]; };
#elif MCC_VERTEX_COMPRESSION == _MLTR
layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_meshlet_coding vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { uvec4 bone_indices_lut[]; };
#extension GL_EXT_control_flow_attributes : enable
#include "blend_attribute_compression.glsl"
#endif

#if MCC_VERTEX_COMPRESSION == _NOCOMP
vertex_data getVertexData(uint vid) {
    vertex_data ret;
    ret.mPosition = vertices[vid].mPositionTxX.xyz;
    ret.mNormal = vertices[vid].mTxYNormal.yzw;
    ret.mTexCoord = vec2(vertices[vid].mPositionTxX.w, vertices[vid].mTxYNormal.x);
    ret.mBoneIndices = vertices[vid].mBoneIndices;
    ret.mBoneWeights = vertices[vid].mBoneWeights;
    return ret;
}

#elif MCC_VERTEX_COMPRESSION == _LUT
vertex_data getVertexData(uint vid) {
    vertex_data ret;
    ret.mPosition = vertices[vid].mPositionTxX.xyz;
    ret.mNormal = vertices[vid].mTxYNormal.yzw;
    ret.mTexCoord = vec2(vertices[vid].mPositionTxX.w, vertices[vid].mTxYNormal.x);
    ret.mBoneIndices = bone_indices_lut[vertices[vid].mBoneIndicesLUID];
    ret.mBoneWeights = vec4(vertices[vid].mBoneWeights, 1.0);
    ret.mBoneWeights.w = 1.0 - ( ret.mBoneWeights.x + ret.mBoneWeights.y + ret.mBoneWeights.z );
    return ret;
}

#elif MCC_VERTEX_COMPRESSION == _MLTR
// https://twitter.com/Stubbesaurus/status/937994790553227264
// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 octahedronDecode( vec2 f )
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ));
    float t = clamp(-n.z, 0.0, 1.0);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize( n );
}

vertex_data getVertexData(uint vid) {
    vertex_data ret;
    uvec3 disPosition = uvec3(
        bitfieldExtract(vertices[vid].mPosition.x, 16, 16),
        bitfieldExtract(vertices[vid].mPosition.x, 0, 16),
        bitfieldExtract(vertices[vid].mPosition.y, 16, 16)
    );
    ret.mPosition = disPosition * (1.0 / 0xFFFF);
    ret.mNormal = octahedronDecode(vec2(
        bitfieldExtract(vertices[vid].mPosition.z, 16, 16) / 65535.0,
        bitfieldExtract(vertices[vid].mPosition.z, 0, 16) / 65535.0
    ));

    ret.mTexCoord = vec2(
        bitfieldExtract(vertices[vid].mPosition.w, 16, 16) / 65535.0,
        bitfieldExtract(vertices[vid].mPosition.w, 0, 16) / 65535.0
    );

    uvec2 code = uvec2(vertices[vid].mBoneIndicesLUID.y, 0);
    bool valid; // not really necessary...
    blend_attribute_codec_t codec;
    codec.weight_value_count = 18;
    codec.extra_value_counts[0] = 1;
    codec.extra_value_counts[1] = 1;
    codec.extra_value_counts[2] = 2;
    codec.payload_value_count_over_factorial = 0; // irrelevant (only for valid check)
    float out_weights[ENTRY_COUNT + 1];
    uint tuple_index = decompress_blend_attributes(out_weights, valid, code, codec);

    ret.mBoneWeights.w = out_weights[0];
    ret.mBoneWeights.z = out_weights[1];
    ret.mBoneWeights.y = out_weights[2];
    ret.mBoneWeights.x = out_weights[3]; 
    ret.mBoneIndices = bone_indices_lut[tuple_index];
    //ret.mBoneIndices = bone_indices_lut[vertices[vid].mBoneIndicesLUID.x];
    //ret.mBoneWeights = vertices[vid].mBoneWeights;
    //ret.mBoneWeights.w = 1.0 - ( ret.mBoneWeights.x + ret.mBoneWeights.y + ret.mBoneWeights.z );
    return ret;
}

#endif

