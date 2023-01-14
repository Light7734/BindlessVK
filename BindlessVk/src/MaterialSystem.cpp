#include "BindlessVk/MaterialSystem.hpp"

#include <spirv_reflect.h>

static_assert(
    SPV_REFLECT_RESULT_SUCCESS == false,
    "SPV_REFLECT_RESULT_SUCCESS was suppoed to be 0 (false), but it isn't"
);

namespace BINDLESSVK_NAMESPACE {

void MaterialSystem::init(Device* device)
{
	this->device = device;

	vec<vk::DescriptorPoolSize> pool_sizes = { { vk::DescriptorType::eSampler, 1000 },
		                                       { vk::DescriptorType::eCombinedImageSampler, 1000 },
		                                       { vk::DescriptorType::eSampledImage, 1000 },
		                                       { vk::DescriptorType::eStorageImage, 1000 },
		                                       { vk::DescriptorType::eUniformTexelBuffer, 1000 },
		                                       { vk::DescriptorType::eStorageTexelBuffer, 1000 },
		                                       { vk::DescriptorType::eUniformBuffer, 1000 },
		                                       { vk::DescriptorType::eStorageBuffer, 1000 },
		                                       { vk::DescriptorType::eUniformBufferDynamic, 1000 },
		                                       { vk::DescriptorType::eStorageBufferDynamic, 1000 },
		                                       { vk::DescriptorType::eInputAttachment, 1000 } };
	// descriptorPoolSizes.push_back(VkDescriptorPoolSize {});

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info {
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		100,
		static_cast<uint32_t>(pool_sizes.size()),
		pool_sizes.data(),
	};

	descriptor_pool = device->logical.createDescriptorPool(descriptor_pool_create_info, nullptr);

	const vk::Extent2D& extent = device->framebuffer_extent;
}

void MaterialSystem::reset()
{
	destroy_all_materials();

	device->logical.destroyDescriptorPool(descriptor_pool);

	for (auto& [key, val] : shader_effects)
	{
		device->logical.destroyDescriptorSetLayout(val.sets_layout[0]);
		device->logical.destroyDescriptorSetLayout(val.sets_layout[1]);
		device->logical.destroyPipelineLayout(val.pipeline_layout);

		static_assert(ShaderEffect().sets_layout.size() == 2, "Sets layout has been resized");
	}

	for (auto& [key, val] : shaders)
	{
		device->logical.destroyShaderModule(val.module);
	}


	shader_effects.clear();
	shaders.clear();
}

void MaterialSystem::destroy_all_materials()
{
	for (auto& [key, val] : shader_passes)
	{
		device->logical.destroyPipeline(val.pipeline);
	}

	device->logical.resetDescriptorPool(descriptor_pool);

	shader_passes.clear();
	materials.clear();
}

void MaterialSystem::load_shader(const char* name, const char* path, vk::ShaderStageFlagBits stage)
{
	std::ifstream stream(path, std::ios::ate);

	const size_t file_size = stream.tellg();
	vec<uint32_t> code(file_size / sizeof(uint32_t));

	stream.seekg(0);
	stream.read((char*)code.data(), file_size);
	stream.close();

	vk::ShaderModuleCreateInfo createInfo {
		{},                             // flags
		code.size() * sizeof(uint32_t), // codeSize
		code.data(),                    // code
	};

	shaders[hash_str(name)] = {
		device->logical.createShaderModule(createInfo), // module
		stage,                                          // stage
		code,                                           // code
	};
}

void MaterialSystem::create_shader_effect(const char* name, vec<Shader*> shaders)
{
	arr<vec<vk::DescriptorSetLayoutBinding>, 2> set_bindings;

	for (const auto& shader_stage : shaders)
	{
		SpvReflectShaderModule spv_module;
		assert_false(
		    spvReflectCreateShaderModule(
		        shader_stage->code.size() * sizeof(uint32_t),
		        shader_stage->code.data(),
		        &spv_module
		    ),
		    "spvReflectCreateShderModule failed"
		);

		uint32_t descriptor_sets_count = 0ul;

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
			for (uint32_t i_binding = 0ull; i_binding < spv_set->binding_count; i_binding++)
			{
				const auto& spv_binding = *(spv_set->bindings[i_binding]);
				if (set_bindings[spv_set->set].size() < spv_binding.binding + 1)
				{
					set_bindings[spv_set->set].resize(spv_binding.binding + 1);
				}

				set_bindings[spv_binding.set][spv_binding.binding].binding = spv_binding.binding;

				set_bindings[spv_binding.set][spv_binding.binding].descriptorType =
				    static_cast<vk::DescriptorType>(spv_binding.descriptor_type);

				set_bindings[spv_binding.set][spv_binding.binding].stageFlags = shader_stage->stage;

				set_bindings[spv_binding.set][spv_binding.binding].descriptorCount = 1u;
				for (uint32_t i_dim = 0; i_dim < spv_binding.array.dims_count; i_dim++)
				{
					set_bindings[spv_binding.set][spv_binding.binding].descriptorCount *=
					    spv_binding.array.dims[i_dim];
				}
			}
		}
	}

	arr<vk::DescriptorSetLayout, 2ull> sets_layout;
	uint32_t index = 0ull;
	for (const auto& set : set_bindings)
	{
		vk::DescriptorSetLayoutCreateInfo set_layout_info {
			{},                                // flags
			static_cast<uint32_t>(set.size()), // bindingCount
			set.data(),                        // pBindings
		};
		sets_layout[index++] = device->logical.createDescriptorSetLayout(set_layout_info);
	}
	vk::PipelineLayoutCreateInfo pipeline_layout_info = {
		{},                                        // flags
		static_cast<uint32_t>(sets_layout.size()), // setLayoutCount
		sets_layout.data(),                        // pSetLayouts
	};


	shader_effects[hash_str(name)] = {
		shaders,                                                    // shaders
		device->logical.createPipelineLayout(pipeline_layout_info), // piplineLayout
		sets_layout                                                 // setsLayout
	};
}

void MaterialSystem::create_shader_pass(
    const char* name,
    ShaderEffect* effect,
    vk::Format color_attachment_format,
    vk::Format depth_attachment_format,
    PipelineConfiguration pipeline_configuration
)
{
	if (shader_passes.contains(hash_str(name)))
	{
		device->debug_callback(LogLvl::eWarn, fmt::format("Recreating shader pass: {}", name));
		device->logical.destroyPipeline(shader_passes[hash_str(name)].pipeline);
	}

	vec<vk::PipelineShaderStageCreateInfo> stages(effect->shaders.size());

	uint32_t index = 0;
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
		{},
		1u,
		&color_attachment_format,
		depth_attachment_format, // :D
		{},
	};

	vk::GraphicsPipelineCreateInfo graphics_pipeline_info {
		{},
		static_cast<uint32_t>(stages.size()),
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

	shader_passes[hash_str(name)] = {
		effect,
		pipeline.value,
	};
}

void MaterialSystem::create_pipeline_configuration(
    const char* name,
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
	PipelineConfiguration& configuration = pipline_configurations[hash_str(name)];


	configuration = PipelineConfiguration {
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
		static_cast<uint32_t>(configuration.dynamic_states.size()),
		configuration.dynamic_states.data(),
	};

	configuration.color_blend_state.attachmentCount = configuration.color_blend_attachments.size();
	configuration.color_blend_state.pAttachments = configuration.color_blend_attachments.data();
}

void MaterialSystem::create_material(
    const char* name,
    ShaderPass* shader_pass,
    MaterialParameters parameters,
    vec<class Texture*> textures
)
{
	vk::DescriptorSetAllocateInfo allocate_info {
		descriptor_pool, // descriptorPool
		1ull,            // descriptorSetCount
		&shader_pass->effect->sets_layout.back(),
	};

	if (materials.contains(hash_str(name)))
	{
		device->logical.freeDescriptorSets(
		    descriptor_pool,
		    materials[hash_str(name)].descriptor_set
		);
		device->debug_callback(LogLvl::eWarn, fmt::format("Recreating material: {}", name));
	}

	vk::DescriptorSet set;
	assert_false(device->logical.allocateDescriptorSets(&allocate_info, &set));

	materials[hash_str(name)] = {
		.shader_pass = shader_pass,
		.parameters = parameters,
		.descriptor_set = set,
		.textures = textures,
		.sort_key = static_cast<uint32_t>(hash_str(name)),
	};

	// @todo Bind textures
}

} // namespace BINDLESSVK_NAMESPACE
