#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

#include "global_descriptors.glsl"

layout(location = 0) in vec3 in_uvw;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(s_texture_cubes[0],  in_uvw);
}
