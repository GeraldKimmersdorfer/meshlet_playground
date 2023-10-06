
#if MCC_VERTEX_COMPRESSION == _NOCOMP
layout(set = 3, binding = 0) buffer VertexBuffer { vertex_data vertices[]; };
#elif MCC_VERTEX_COMPRESSION == _LUT
layout(set = 3, binding = 0, scalar) buffer VertexBuffer { vertex_data_bone_lookup vertices[]; };
layout(set = 3, binding = 1) buffer BoneIndicesLUT { uvec4 bone_indices_lut[]; };
#endif

#if MCC_VERTEX_COMPRESSION == _NOCOMP
vertex_data getVertexData(uint vid) {
    return vertices[vid];
}

#elif MCC_VERTEX_COMPRESSION == _LUT
vertex_data getVertexData(uint vid) {
    vertex_data ret;
    ret.mPositionTxX = vertices[vid].mPositionTxX;
    ret.mTxYNormal = vertices[vid].mTxYNormal;
    ret.mBoneIndices = bone_indices_lut[vertices[vid].mBoneIndicesLUID];
    //ret.mBoneIndices = uvec4(vertices[vid].mBoneIndicesLUID);
    ret.mBoneWeights = vec4(vertices[vid].mBoneWeights, 1.0);
    //ret.mBoneWeights.a = 1.0 - ( ret.mBoneWeights.x + ret.mBoneWeights.y + ret.mBoneWeights.z );
    return ret;
}
uint getLUID(uint vid) {
    return vertices[vid].mBoneIndicesLUID;
}
#endif
