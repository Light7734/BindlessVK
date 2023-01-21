#include "BindlessVk/MaterialSystem.hpp"

#include <spirv_reflect.h>

static_assert(
    SPV_REFLECT_RESULT_SUCCESS == false,
    "SPV_REFLECT_RESULT_SUCCESS was suppoed to be 0 (false), but it isn't"
);

namespace BINDLESSVK_NAMESPACE {

MaterialSystem::MaterialSystem(Device* device): device(device)
{
	vec<vk::DescriptorPoolSize> pool_sizes = { { vk::DescriptorType::eSampler, 1000u },
		                                       { vk::DescriptorType::eCombinedImageSampler, 1000u },
		                                       { vk::DescriptorType::eSampledImage, 1000u },
		                                       { vk::DescriptorType::eStorageImage, 1000u },
		                                       { vk::DescriptorType::eUniformTexelBuffer, 1000u },
		                                       { vk::DescriptorType::eStorageTexelBuffer, 1000u },
		                                       { vk::DescriptorType::eUniformBuffer, 1000u },
		                                       { vk::DescriptorType::eStorageBuffer, 1000u },
		                                       { vk::DescriptorType::eUniformBufferDynamic, 1000u },
		                                       { vk::DescriptorType::eStorageBufferDynamic, 1000u },
		                                       { vk::DescriptorType::eInputAttachment, 1000u } };

	descriptor_pool = device->logical.createDescriptorPool(
	    vk::DescriptorPoolCreateInfo {
	        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	        100u,
	        static_cast<u32>(pool_sizes.size()),
	        pool_sizes.data(),
	    },
	    nullptr
	);
}

MaterialSystem::~MaterialSystem()
{
}

Shader MaterialSystem::load_shader(c_str name, c_str path, vk::ShaderStageFlagBits stage)
{
	std::ifstream stream(path, std::ios::ate);

	const usize file_size = stream.tellg();
	vec<u32> code(file_size / sizeof(u32));

	stream.seekg(0);
	stream.read((char*)code.data(), file_size);
	stream.close();

	return Shader {
		device->logical.createShaderModule(vk::ShaderModuleCreateInfo {
		    {},
		    code.size() * sizeof(u32),
		    code.data(),
		}),
		stage,
		code,
	};
}

ShaderEffect MaterialSystem::create_shader_effect(c_str name, vec<Shader*> shaders)
{
	arr<vec<vk::DescriptorSetLayoutBinding>, 2u> set_bindings;

	for (const auto& shader_stage : shaders)
	{
		SpvReflectShaderModule spv_module;
		assert_false(
		    spvReflectCreateShaderModule(
		        shader_stage->code.size() * sizeof(u32),
		        shader_stage->code.data(),
		        &spv_module
		    ),
		    "spvReflectCreateShderModule failed"
		);

		u32 descriptor_sets_count = 0u;

		assert_false(
		    spvReflectEnumerateDescriptorSets(&spv_module, &descriptor_sets_count, nullptr),
		    "spvReflectEnumerateDescriptorSets failed"
		);

		vec<SpvReflectDescriptorSet*> descriptor_sets(descriptor_sets_count);

		assert_false(
		    spvReflectEnumerateDescriptorSets(
		        &spv_module,
		        &descriptor_sets_count,
		        descriptor_sets.data()
		    ),
		    "spvReflectEnumerateDescriptorSets failed"
		);

		for (const auto& spv_set : descriptor_sets)
		{
			for (u32 i_binding = 0u; i_binding < spv_set->binding_count; ++i_binding)
			{
				const auto& spv_binding = *(spv_set->bindings[i_binding]);
				if (set_bindings[spv_set->set].size() < spv_binding.binding + 1u)
				{
					set_bindings[spv_set->set].resize(spv_binding.binding + 1u);
				}

				set_bindings[spv_binding.set][spv_binding.binding].binding = spv_binding.binding;

				set_bindings[spv_binding.set][spv_binding.binding].descriptorType =
				    static_cast<vk::DescriptorType>(spv_binding.descriptor_type);

				set_bindings[spv_binding.set][spv_binding.binding].stageFlags = shader_stage->stage;

				set_bindings[spv_binding.set][spv_binding.binding].descriptorCount = 1u;
				for (u32 i_dim = 0; i_dim < spv_binding.array.dims_count; ++i_dim)
				{
					set_bindings[spv_binding.set][spv_binding.binding].descriptorCount *=
					    spv_binding.array.dims[i_dim];
				}
			}
		}
	}

	arr<vk::DescriptorSetLayout, 2u> sets_layout;
	u32 index = 0u;
	for (const auto& set : set_bindings)
	{
		sets_layout[index++] =
		    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {
		        {},
		        static_cast<u32>(set.size()),
		        set.data(),
		    });
	}
	vk::PipelineLayoutCreateInfo pipeline_layout_info = {
		{},
		static_cast<u32>(sets_layout.size()),
		sets_layout.data(),
	};

	return ShaderEffect {
		shaders,
		device->logical.createPipelineLayout(pipeline_layout_info),
		sets_layout,
	};
}

ShaderPass MaterialSystem::create_shader_pass(
    c_str name,
    ShaderEffect* effect,
    vk::Format color_attachment_format,
    vk::Format depth_attachment_format,
    PipelineConfiguration pipeline_configuration
)
{
	assert_true(effect, "effect is null for '{}' material", name);
	vec<vk::PipelineShaderStageCreateInfo> stages(effect->shaders.size());

	u32 index = 0;
	for (const auto& shader : effect->shaders)
	{
		stages[index++] = {
			{},
			shader->stage,
			shader->module,
			"main",
		};
	}

	vk::PipelineRenderingCreateInfo pipeline_rendering_info {
		{}, 1u, &color_attachment_format, depth_attachment_format, {},
	};

	vk::GraphicsPipelineCreateInfo graphics_pipeline_info {
		{},
		static_cast<u32>(stages.size()),
		stages.data(),
		&pipeline_configuration.vertex_input_state,
		&pipeline_configuration.input_assembly_state,
		&pipeline_configuration.tesselation_state,
		&pipeline_configuration.viewport_state,
		&pipeline_configuration.rasterization_state,
		&pipeline_configuration.multisample_state,
		&pipeline_configuration.depth_stencil_state,
		&pipeline_configuration.color_blend_state,
		&pipeline_configuration.dynamic_state,
		effect->pipeline_layout,
		{},
		{},
		{},
		{},
		&pipeline_rendering_info,
	};

	auto pipeline = device->logical.createGraphicsPipeline({}, graphics_pipeline_info);
	assert_false(pipeline.result);

	return ShaderPass {
		effect,
		pipeline.value,
	};
}

PipelineConfiguration MaterialSystem::create_pipeline_configuration(
    c_str name,
    vk::PipelineVertexInputStateCreateInfo vertex_input_state,
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state,
    vk::PipelineTessellationStateCreateInfo tessellation_state,
    vk::PipelineViewportStateCreateInfo viewport_state,
    vk::PipelineRasterizationStateCreateInfo rasterization_state,
    vk::PipelineMultisampleStateCreateInfo multisample_state,
    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state,
    vec<vk::PipelineColorBlendAttachmentState> color_blend_attachments,
    vk::PipelineColorBlendStateCreateInfo color_blend_state,
    vec<vk::DynamicState> dynamic_states
)
{
	auto configuration = PipelineConfiguration {
		.vertex_input_state = vertex_input_state,
		.input_assembly_state = input_assembly_state,
		.tesselation_state = tessellation_state,
		.viewport_state = viewport_state,
		.rasterization_state = rasterization_state,
		.multisample_state = multisample_state,
		.depth_stencil_state = depth_stencil_state,

		.color_blend_attachments = color_blend_attachments,
		.color_blend_state = color_blend_state,

		.dynamic_states = dynamic_states,
	};

	configuration.dynamic_state = {
		{},
		static_cast<u32>(configuration.dynamic_states.size()),
		configuration.dynamic_states.data(),
	};

	configuration.color_blend_state.attachmentCount = configuration.color_blend_attachments.size();
	configuration.color_blend_state.pAttachments = configuration.color_blend_attachments.data();

	return configuration;
}

Material MaterialSystem::create_material(
    c_str name,
    ShaderPass* shader_pass,
    MaterialParameters parameters,
    vec<class Texture*> textures
)
{
	vk::DescriptorSetAllocateInfo allocate_info {
		descriptor_pool,
		1ul,
		&shader_pass->effect->sets_layout.back(),
	};

	vk::DescriptorSet set;
	assert_false(device->logical.allocateDescriptorSets(&allocate_info, &set));

	return Material {
		.shader_pass = shader_pass,
		.parameters = parameters,
		.descriptor_set = set,
		.textures = textures,
		.sort_key = static_cast<u32>(hash_str(name)),
	};
}

} // namespace BINDLESSVK_NAMESPACE
