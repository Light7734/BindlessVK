#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D uTexSampler;

void main()
{
	fragColor = texture(uTexSampler, fragUV);
}