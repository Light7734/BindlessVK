#include "BindlessVk/MaterialSystem.hpp"

#include <ranges>
#include <spirv_reflect.h>

static_assert(
    SPV_REFLECT_RESULT_SUCCESS == false,
    "SPV_REFLECT_RESULT_SUCCESS was suppoed to be 0 (false), but it isn't"
);

namespace BINDLESSVK_NAMESPACE {

MaterialSystem::MaterialSystem(Device* device): device(device)
{
	vec<vk::DescriptorPoolSize> pool_sizes = {
		{ vk::DescriptorType::eSampler, 1000u },
		{ vk::DescriptorType::eCombinedImageSampler, 1000u },
		{ vk::DescriptorType::eSampledImage, 1000u },
		{ vk::DescriptorType::eStorageImage, 1000u },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000u },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000u },
		{ vk::DescriptorType::eUniformBuffer, 1000u },
		{ vk::DescriptorType::eStorageBuffer, 1000u },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000u },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000u },
		{ vk::DescriptorType::eInputAttachment, 1000u },
	};

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
	auto shader_code = load_shader_code(path);

	return Shader {
		device->logical.createShaderModule(vk::ShaderModuleCreateInfo {
		    {},
		    shader_code.size() * sizeof(u32),
		    shader_code.data(),
		}),
		stage,
		shader_code,
	};
}

ShaderEffect MaterialSystem::create_shader_effect(c_str name, vec<Shader*> shaders)
{
	auto sets_bindings = reflect_shader_effect_bindings(shaders);
	auto sets_layout = create_descriptor_sets_layout(sets_bindings);

	return ShaderEffect {
		shaders,
		device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo {
		    {},
		    static_cast<u32>(sets_layout.size()),
		    sets_layout.data(),
		}),
		sets_layout,
	};
}

ShaderPass MaterialSystem::create_shader_pass(
    c_str name,
    ShaderEffect* shader_effect,
    vk::Format color_attachment_format,
    vk::Format depth_attachment_format,
    PipelineConfiguration pipeline_configuration
)
{
	auto pipeline_layout = shader_effect->pipeline_layout;
	auto pipeline_shader_stage_create_infos = create_pipeline_shader_stage_infos(shader_effect);
	auto pipeline_rendering_info = vk::PipelineRenderingCreateInfo {
		{},
		1u,
		&color_attachment_format,
		depth_attachment_format, //
		{},
	};

	const auto graphics_pipeline = create_graphics_pipeline(
	    pipeline_shader_stage_create_infos,
	    pipeline_rendering_info,
	    pipeline_configuration,
	    pipeline_layout
	);

	return ShaderPass {
		shader_effect,
		graphics_pipeline,
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
		.descriptor_set = set,
		.textures = textures,
		.sort_key = static_cast<u32>(hash_str(name)),
	};
}


vec<u32> MaterialSystem::load_shader_code(c_str path)
{
	std::ifstream stream(path, std::ios::ate);
	const usize file_size = stream.tellg();
	vec<u32> code(file_size / sizeof(u32));

	stream.seekg(0);
	stream.read((char*)code.data(), file_size);
	stream.close();

	return code;
}

arr<vec<vk::DescriptorSetLayoutBinding>, 2u> MaterialSystem::reflect_shader_effect_bindings(
    vec<Shader*> shaders
)
{
	arr<vec<vk::DescriptorSetLayoutBinding>, 2u> bindings = {};

	for (const auto& shader : shaders)
	{
		auto spv_sets = reflect_spv_descriptor_sets(shader);

		for (const auto& spv_set : spv_sets)
		{
			for (u32 i_binding = 0u; i_binding < spv_set->binding_count; ++i_binding)
			{
				const auto& spv_binding = *spv_set->bindings[i_binding];
				if (bindings[spv_set->set].size() < spv_binding.binding + 1u)
				{
					bindings[spv_set->set].resize(spv_binding.binding + 1u);
				}

				const u32 set_index = spv_binding.set;
				const u32 binding_index = spv_binding.binding;

				bindings[set_index][binding_index] =
				    extract_descriptor_binding(spv_set->bindings[i_binding], shader);
			}
		}
	}

	return bindings;
}

arr<vk::DescriptorSetLayout, 2> MaterialSystem::create_descriptor_sets_layout(
    arr<vec<vk::DescriptorSetLayoutBinding>, 2> sets_bindings
)
{
	arr<vk::DescriptorSetLayout, 2> sets_layout;

	for (u32 i = 0u; const auto& set_bindings : sets_bindings)
	{
		sets_layout[i++] =
		    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {
		        {},
		        static_cast<u32>(set_bindings.size()),
		        set_bindings.data(),
		    });
	}

	return sets_layout;
}

vec<vk::PipelineShaderStageCreateInfo> MaterialSystem::create_pipeline_shader_stage_infos(
    ShaderEffect* shader_effect
)
{
	vec<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_infos;
	pipeline_shader_stage_infos.resize(shader_effect->shaders.size());

	for (u32 i = 0; const auto& shader : shader_effect->shaders)
	{
		pipeline_shader_stage_infos[i] = {
			{},
			shader->stage,
			shader->module,
			"main",
		};
	}

	return pipeline_shader_stage_infos;
}

vk::Pipeline MaterialSystem::create_graphics_pipeline(
    vec<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos,
    const vk::PipelineRenderingCreateInfoKHR& rendering_info,
    const PipelineConfiguration& configuration,
    vk::PipelineLayout layout
)
{
	vk::GraphicsPipelineCreateInfo graphics_pipeline_info {
		{},
		static_cast<u32>(shader_stage_create_infos.size()),
		shader_stage_create_infos.data(),
		&configuration.vertex_input_state,
		&configuration.input_assembly_state,
		&configuration.tesselation_state,
		&configuration.viewport_state,
		&configuration.rasterization_state,
		&configuration.multisample_state,
		&configuration.depth_stencil_state,
		&configuration.color_blend_state,
		&configuration.dynamic_state,
		layout,
		{},
		{},
		{},
		{},
		&rendering_info,
	};

	auto pipeline = device->logical.createGraphicsPipeline({}, graphics_pipeline_info);
	assert_false(pipeline.result);

	return pipeline.value;
}

vec<SpvReflectDescriptorSet*> MaterialSystem::reflect_spv_descriptor_sets(Shader* shader)
{
	SpvReflectShaderModule spv_module;
	assert_false(
	    spvReflectCreateShaderModule(
	        shader->code.size() * sizeof(u32),
	        shader->code.data(),
	        &spv_module
	    ),
	    "spvReflectCreateShderModule failed"
	);

	u32 descriptor_sets_count = 0;

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

	return descriptor_sets;
}

vk::DescriptorSetLayoutBinding MaterialSystem::extract_descriptor_binding(
    SpvReflectDescriptorBinding* spv_binding,
    Shader* shader
)
{
	vk::DescriptorSetLayoutBinding binding;
	binding = spv_binding->binding;

	binding.descriptorType = static_cast<vk::DescriptorType>(spv_binding->descriptor_type);

	binding.stageFlags = shader->stage;

	binding.descriptorCount = 1u;
	for (u32 i_dim = 0; i_dim < spv_binding->array.dims_count; ++i_dim)
	{
		binding.descriptorCount *= spv_binding->array.dims[i_dim];
	}

	return binding;
	;
}

} // namespace BINDLESSVK_NAMESPACE
