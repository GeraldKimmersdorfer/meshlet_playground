/*
 * Copyright (c) 2024, Gerald Kimmersdorfer
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
 
#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);


void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}