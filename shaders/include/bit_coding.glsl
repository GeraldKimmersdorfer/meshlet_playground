
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

uvec2 encode_position_2x32(vec3 value) {
    uint x = uint(value.x * float((1u << 21u) - 1u));
    uint y = uint(value.y * float((1u << 21u) - 1u));
    uint z = uint(value.z * float((1u << 21u) - 1u));

    return uvec2((x << 11u) | (y >> 10u), (y << 22u) | z);
}

vec3 decode_position_2x32(uvec2 value) {
    uint x = (value.x >> 11u) & ((1u << 21u) - 1u);
    uint y = ((value.x << 10u) | (value.y >> 22u)) & ((1u << 21u) - 1u);
    uint z = value.y & ((1u << 21u) - 1u);

    return vec3(float(x) / float((1u << 21u) - 1u), 
                float(y) / float((1u << 21u) - 1u), 
                float(z) / float((1u << 21u) - 1u));
}

// GLSL Function to pack mbiluid and permutation
uint packMbiluidAndPermutation(uint mbiluid, uint permutation) {
    return (mbiluid << 5) | permutation;
}

// GLSL Function to unpack into mbiluid and permutation
void unpackMbiluidAndPermutation(uint packedValue, out uint mbiluid, out uint permutation) {
    mbiluid = (packedValue >> 5) & 0x03;
    permutation = packedValue & 0x1F;
}