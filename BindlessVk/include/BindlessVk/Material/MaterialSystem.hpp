#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Shader/Shader.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {
/// @todo
/** @brief Shaders and pipeline layout for creating ShaderPasses
 * usually a pair of vertex and fragment shaders
 */

class Material
{
public:
	struct Parameters
	{
		arr<f32, 4> albedo;
		arr<f32, 4> emissive;
		arr<f32, 4> diffuse;
		arr<f32, 4> specular;
		f32 metallic;
		f32 roughness;
	};

public:
	Material(VkContext *vk_context, ShaderPipeline *effect, vk::DescriptorPool descriptor_pool);

	~Material() = default;

	inline auto *get_effect() const
	{
		return effect;
	}

	inline auto get_descriptor_set() const
	{
		return descriptor_set;
	}

private:
	ShaderPipeline *effect = {};
	Parameters parametes = {};
	vk::DescriptorSet descriptor_set = {};
};

} // namespace BINDLESSVK_NAMESPACE
