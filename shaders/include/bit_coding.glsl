/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
 
float decode8(uint value) { return float(value) / 255.0; }
vec2 decode8(uvec2 value) { return vec2(float(value.x) / 255.0, float(value.y) / 255.0); }
vec3 decode8(uvec3 value) { return vec3(float(value.x) / 255.0, float(value.y) / 255.0, float(value.z) / 255.0); }
vec4 decode8(uvec4 value) { return vec4(float(value.x) / 255.0, float(value.y) / 255.0, float(value.z) / 255.0, float(value.w) / 255.0); }

float decode16(uint value) { return float(value) / 65535.0; }
vec2 decode16(uvec2 value) { return vec2(float(value.x) / 65535.0, float(value.y) / 65535.0); }
vec3 decode16(uvec3 value) { return vec3(float(value.x) / 65535.0, float(value.y) / 65535.0, float(value.z) / 65535.0); }
vec4 decode16(uvec4 value) { return vec4(float(value.x) / 65535.0, float(value.y) / 65535.0, float(value.z) / 65535.0, float(value.w) / 65535.0); }

float decode21(uint value) { return float(value) / 2097151.0; }
vec2 decode21(uvec2 value) { return vec2(float(value.x) / 2097151.0, float(value.y) / 2097151.0); }
vec3 decode21(uvec3 value) { return vec3(float(value.x) / 2097151.0, float(value.y) / 2097151.0, float(value.z) / 2097151.0); }
vec4 decode21(uvec4 value) { return vec4(float(value.x) / 2097151.0, float(value.y) / 2097151.0, float(value.z) / 2097151.0, float(value.w) / 2097151.0); }

uint concatenate2x8(uint low, uint high) { return (high << 8) | low; }
uint concatenate3x8(uint low, uint mid, uint high) { return (high << 16) | (mid << 8) | low; }
uint concatenate4x8(uint low, uint midLow, uint midHigh, uint high) { return (high << 24) | (midHigh << 16) | (midLow << 8) | low; }
uint concatenate2x16(uint low, uint high) { return (high << 16) | low; }

uint concatenate16a8(uint low, uint high) { return (high << 16) | low; }

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
    return decode21(uvec3(
        (value.x >> 11u) & ((1u << 21u) - 1u),
        ((value.x << 10u) | (value.y >> 22u)) & ((1u << 21u) - 1u),
        value.y & ((1u << 21u) - 1u)
    ));
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

