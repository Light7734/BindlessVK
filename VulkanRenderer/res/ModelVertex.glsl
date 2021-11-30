#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

layout(binding = 0) uniform ubo_MVP
{
	mat4 model;
	mat4 view;
	mat4 projection;
} MVP;

void main()
{
	gl_Position = MVP.projection * MVP.view * MVP.model * vec4(inPosition, 1.0f);

	fragUV = inUV;
}