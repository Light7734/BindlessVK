#pragma once

#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Shader/DescriptorSet.hpp"
#include "BindlessVk/Shader/Shader.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Represents shading properties of models */
class Material
{
public:
	/** Parameters that affect the shading of models */
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
	/** Default constructor */
	Material() = default;

	/** Argumented constructor
	 *
	 * @param
	 */
	Material(
	    DescriptorAllocator *descriptor_allocator,
	    ShaderPipeline *shader_pipeline,
	    vk::DescriptorPool descriptor_pool
	);

	/** Default move constructor */
	Material(Material &&other) = default;

	/** Default move assignment operator */
	Material &operator=(Material &&other) = default;

	/** Deleted copy constructor */
	Material(Material const &) = delete;

	/** Deleted copy assignment operator */
	Material &operator=(Material const &) = delete;

	/** Default destructor */
	~Material() = default;

	/** Trivial accessor for shade_pipeline */
	auto *get_shader_pipeline() const
	{
		return shader_pipeline;
	}

	/** Accessor for the descriptor set's underlying descriptor set */
	auto get_descriptor_set() const
	{
		return descriptor_set.vk();
	}

private:
	ShaderPipeline *shader_pipeline = {};
	Parameters parameters = {};
	DescriptorSet descriptor_set = {};
};

} // namespace BINDLESSVK_NAMESPACE
