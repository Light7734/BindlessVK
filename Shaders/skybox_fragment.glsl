#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 in_uvw;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 3) uniform samplerCube u_texture_cubes[];

void main()
{
    out_color = texture(u_texture_cubes[0],  in_uvw);
}

