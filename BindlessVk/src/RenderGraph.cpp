#include "BindlessVk/RenderGraph.hpp"

#include "BindlessVk/Texture.hpp"

#include <fmt/format.h>
#include <ranges>

namespace BINDLESSVK_NAMESPACE {

RenderGraphBuilder::RenderGraphBuilder(ref<VkContext> const vk_context): vk_context(vk_context)
{
}

auto RenderGraphBuilder::build_graph() -> Rendergraph*
{
	backbuffer_attachment_key = pass_blueprints.back().color_attachments.back().hash;
	resources->add_key_to_attachment_index(backbuffer_attachment_key, 0);

	build_graph_buffer_inputs();
	build_graph_texture_inputs();
	build_graph_input_descriptors();
	initialize_graph_input_descriptors();

	for (u32 i = pass_blueprints.size(); i-- > 0;)
	{
		auto const &blueprint = pass_blueprints[i];
		auto &pass = graph->passes[i];

		pass->name = blueprint.name;
		pass->sample_count = blueprint.sample_count;
		pass->update_label = blueprint.update_label;
		pass->barrier_label = blueprint.barrier_label;
		pass->render_label = blueprint.render_label;
		pass->user_data = blueprint.user_data;

		build_pass_color_attachments(pass, blueprint.color_attachments);
		build_pass_depth_attachment(pass, blueprint.depth_attachment);
		build_pass_buffer_inputs(pass, blueprint.buffer_inputs);
		build_pass_input_descriptors(pass, blueprint.buffer_inputs, blueprint.texture_inputs);
		build_pass_texture_inputs(pass, blueprint.texture_inputs);
		initialize_pass_input_descriptors(pass, blueprint);
	}

	return graph;
}

void RenderGraphBuilder::build_graph_buffer_inputs()
{
	graph->buffer_inputs.reserve(blueprint_buffer_inputs.size());
	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		graph->buffer_inputs.emplace_back(
		    blueprint_buffer_input.name.c_str(),
		    vk_context.get(),
		    blueprint_buffer_input.type == vk::DescriptorType::eUniformBuffer ?
		        vk::BufferUsageFlagBits::eUniformBuffer :
		        vk::BufferUsageFlagBits::eStorageBuffer,
		    vma::AllocationCreateInfo {
		        vma::AllocationCreateFlagBits::eHostAccessRandom,
		        vma::MemoryUsage::eAutoPreferDevice,
		    },
		    blueprint_buffer_input.size,
		    BVK_MAX_FRAMES_IN_FLIGHT
		);
}

void RenderGraphBuilder::build_graph_texture_inputs()
{
}

void RenderGraphBuilder::build_graph_input_descriptors()
{
	auto const device = vk_context->get_device();
	auto *const descriptor_allocator = vk_context->get_descriptor_allocator();

	vec<vk::DescriptorSetLayoutBinding> bindings;
	bindings.reserve(blueprint_buffer_inputs.size());

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		bindings.emplace_back(
		    blueprint_buffer_input.binding,
		    blueprint_buffer_input.type,
		    blueprint_buffer_input.count,
		    blueprint_buffer_input.stage_mask
		);

	graph->descriptor_set_layout = device.createDescriptorSetLayout({ {}, bindings });
	vk_context->set_object_name(
	    graph->descriptor_set_layout,
	    fmt::format("graph_descriptor_set_layout").c_str()
	);

	graph->pipeline_layout = device.createPipelineLayout({ {}, graph->descriptor_set_layout });
	vk_context->set_object_name(
	    graph->pipeline_layout,
	    fmt::format("graph_pipeline_layout").c_str()
	);

	if (!bindings.empty())
	{
		graph->descriptor_sets.reserve(BVK_MAX_FRAMES_IN_FLIGHT);
		for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			graph->descriptor_sets.emplace_back(
			    descriptor_allocator->allocate_descriptor_set(graph->descriptor_set_layout)
			);

			vk_context->set_object_name(
			    graph->descriptor_sets.back(),
			    fmt::format("graph_descriptor_set_{}", i).c_str()
			);
		}
	}
}

void RenderGraphBuilder::initialize_graph_input_descriptors()
{
	auto const device = vk_context->get_device();

	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};
	buffer_infos.reserve(blueprint_buffer_inputs.size() * BVK_MAX_FRAMES_IN_FLIGHT);

	for (u32 k = 0; auto &buffer_input : graph->buffer_inputs)
	{
		auto const &blueprint_buffer = blueprint_buffer_inputs[k++];
		auto writes = vec<vk::WriteDescriptorSet> {};

		for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			buffer_infos.push_back({
			    *buffer_input.get_buffer(),
			    buffer_input.get_block_size() * i,
			    buffer_input.get_block_size(),
			});

			for (u32 j = 0; j < blueprint_buffer.count; ++j)
			{
				writes.push_back(vk::WriteDescriptorSet {
				    graph->descriptor_sets[i],
				    blueprint_buffer.binding,
				    j,
				    1u,
				    blueprint_buffer.type,
				    nullptr,
				    &buffer_infos.back(),
				});
			}
		}

		device.updateDescriptorSets(writes.size(), writes.data(), 0u, nullptr);
		device.waitIdle(); // @todo should we wait idle?
	}
}

void RenderGraphBuilder::build_pass_color_attachments(
    Renderpass *const pass,
    vec<RenderpassBlueprint::Attachment> const &blueprint_attachments
)
{
	pass->attachments.resize(blueprint_attachments.size());
	u32 i = 0;
	for (auto const &blueprint_attachment : blueprint_attachments)
	{
		auto &attachment = pass->attachments[i++];
		pass->color_attachment_formats.push_back(blueprint_attachment.format);

		attachment.stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		attachment.access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
		attachment.image_layout = vk::ImageLayout::eColorAttachmentOptimal;
		attachment.subresource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
		attachment.store_op = vk::AttachmentStoreOp::eStore;
		attachment.clear_value = blueprint_attachment.clear_value;

		auto const has_input = !!blueprint_attachment.input_hash;
		attachment.load_op = has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;

		attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

		if (attachment.resource_index == std::numeric_limits<u32>::max())
		{
			resources->create_color_attachment(blueprint_attachment, pass->sample_count);

			attachment.resource_index =
			    resources->try_get_attachment_index(blueprint_attachment.hash);

			assert_false(attachment.resource_index == std::numeric_limits<u32>::max());
		}
		else
			resources->add_key_to_attachment_index(
			    blueprint_attachment.input_hash,
			    attachment.resource_index
			);

		if (pass->is_multisampled())
		{
			attachment.transient_resolve_moode = vk::ResolveModeFlagBits::eAverage;

			attachment.transient_resource_index =
			    resources->try_get_suitable_transient_attachment_index(
			        blueprint_attachment,
			        pass->sample_count
			    );

			if (attachment.transient_resource_index == std::numeric_limits<u32>::max())
			{
				resources->create_transient_attachment(blueprint_attachment, pass->sample_count);

				attachment.transient_resource_index =
				    resources->try_get_suitable_transient_attachment_index(
				        blueprint_attachment,
				        pass->sample_count
				    );

				assert_false(
				    attachment.transient_resource_index == std::numeric_limits<u32>::max()
				);
			}
		}
	}
}

void RenderGraphBuilder::build_pass_depth_attachment(
    Renderpass *pass,
    RenderpassBlueprint::Attachment const &blueprint_attachment
)
{
	if (!blueprint_attachment)
		return;

	pass->depth_attachment_format = blueprint_attachment.format;

	pass->attachments.push_back({});
	auto &attachment = pass->attachments.back();


	attachment.stage_mask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	attachment.access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
	                         vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	attachment.image_layout = vk::ImageLayout::eDepthAttachmentOptimal;
	attachment.subresource_range = {
		vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1,
	};

	auto const has_input = !!blueprint_attachment.input_hash;
	attachment.load_op = has_input ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;
	attachment.store_op = vk::AttachmentStoreOp::eStore;
	attachment.clear_value = blueprint_attachment.clear_value;

	attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

	if (attachment.resource_index == std::numeric_limits<u32>::max())
	{
		resources->create_depth_attachment(blueprint_attachment, pass->sample_count);

		attachment.resource_index = resources->try_get_attachment_index(blueprint_attachment.hash);

		assert_false(attachment.resource_index == std::numeric_limits<u32>::max());
	}

	else
		resources->add_key_to_attachment_index(
		    attachment.resource_index,
		    blueprint_attachment.input_hash
		);
}

void RenderGraphBuilder::build_pass_buffer_inputs(
    Renderpass *pass,
    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs
)
{
	pass->buffer_inputs.reserve(blueprint_buffer_inputs.size());

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		pass->buffer_inputs.emplace_back(
		    blueprint_buffer_input.name.c_str(),
		    vk_context.get(),
		    blueprint_buffer_input.type == vk::DescriptorType::eUniformBuffer ?
		        vk::BufferUsageFlagBits::eUniformBuffer :
		        vk::BufferUsageFlagBits::eStorageBuffer,

		    vma::AllocationCreateInfo {
		        vma::AllocationCreateFlagBits::eHostAccessRandom,
		        vma::MemoryUsage::eAutoPreferDevice,
		    },

		    blueprint_buffer_input.size,
		    BVK_MAX_FRAMES_IN_FLIGHT
		);
}

void RenderGraphBuilder::build_pass_texture_inputs(
    Renderpass *pass,
    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
)
{
}

void RenderGraphBuilder::build_pass_input_descriptors(
    Renderpass *pass,
    vec<RenderpassBlueprint::BufferInput> const &blueprint_buffer_inputs,
    vec<RenderpassBlueprint::TextureInput> const &blueprint_texture_inputs
)
{
	auto const device = vk_context->get_device();
	auto *const descriptor_allocator = vk_context->get_descriptor_allocator();

	auto bindings = vec<vk::DescriptorSetLayoutBinding> {};
	bindings.reserve(blueprint_buffer_inputs.size() + blueprint_texture_inputs.size());

	for (auto const &blueprint_buffer_input : blueprint_buffer_inputs)
		bindings.emplace_back(
		    blueprint_buffer_input.binding,
		    blueprint_buffer_input.type,
		    blueprint_buffer_input.count,
		    blueprint_buffer_input.stage_mask
		);

	for (auto const &blueprint_texture_input : blueprint_texture_inputs)
		bindings.emplace_back(
		    blueprint_texture_input.binding,
		    blueprint_texture_input.type,
		    blueprint_texture_input.count,
		    blueprint_texture_input.stage_mask
		);

	pass->descriptor_set_layout = device.createDescriptorSetLayout({ {}, bindings });
	vk_context->set_object_name(
	    pass->descriptor_set_layout,
	    fmt::format("{}_descriptor_set_layout", pass->name).c_str()
	);

	auto const layouts = arr<vk::DescriptorSetLayout, 2> {
		graph->descriptor_set_layout,
		pass->descriptor_set_layout,
	};

	// @todo: for some reason, when I use vk_context->set_object_name things break...
	pass->pipeline_layout = device.createPipelineLayout({ {}, layouts });
	device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    pass->pipeline_layout.objectType,
	    (u64)((VkPipelineLayout)(pass->pipeline_layout)),
	    fmt::format("{}_pipeline_layout", pass->name).c_str(),
	});

	if (!bindings.empty())
	{
		pass->descriptor_sets.reserve(BVK_MAX_FRAMES_IN_FLIGHT);
		for (u32 i = i; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			pass->descriptor_sets.emplace_back(
			    descriptor_allocator->allocate_descriptor_set(pass->descriptor_set_layout)
			);

			vk_context->set_object_name(
			    pass->descriptor_sets.back(),
			    fmt::format("{}_descriptor_set_{}", pass->name, i).c_str()
			);
		}
	}
}

void RenderGraphBuilder::initialize_pass_input_descriptors(
    Renderpass *pass,
    RenderpassBlueprint const &pass_blueprint
)
{
	auto const device = vk_context->get_device();

	auto writes = vec<vk::WriteDescriptorSet> {};
	auto buffer_infos = vec<vk::DescriptorBufferInfo> {};

	for (u32 k = 0; auto &buffer : pass->buffer_inputs)
	{
		auto const &blueprint_buffer = pass_blueprint.buffer_inputs[k++];

		for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			buffer_infos.emplace_back(
			    *buffer.get_buffer(),
			    buffer.get_block_size() * i,
			    buffer.get_block_size()
			);

			for (u32 j = 0; j < buffer.get_block_count(); ++j)
			{
				writes.emplace_back(
				    pass->descriptor_sets[i],
				    blueprint_buffer.binding,
				    j,
				    1u,
				    blueprint_buffer.type,
				    nullptr,
				    &buffer_infos.back()
				);
			}
		}
	}

	for (auto const &texture_input_info : pass_blueprint.texture_inputs)
	{
		for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			for (u32 j = 0; j < texture_input_info.count; ++j)
			{
				writes.emplace_back(
				    pass->descriptor_sets[i],
				    texture_input_info.binding,
				    j,
				    1u,
				    texture_input_info.type,
				    &texture_input_info.default_texture->descriptor_info,
				    nullptr
				);
			}
		}
	}

	device.updateDescriptorSets(writes, {});
	device.waitIdle(); // @todo should we wait idle?
}
void RenderGraphBuilder::build_pass_cmd_buffer_begin_infos(Renderpass *pass)
{
	const auto surface = vk_context->get_surface();

	pass->cmd_buffer_inheritance_rendering_info = {
		{},
		{},
		static_cast<u32>(pass->color_attachment_formats.size()),
		pass->color_attachment_formats.data(),
		pass->depth_attachment_format,
		{},
		pass->sample_count,
		{},
	};

	pass->cmd_buffer_inheritance_info = {
		{},
		{},
		{},
		{},
		{},
		{}, // _
		&pass->cmd_buffer_inheritance_rendering_info,
	};

	pass->cmd_buffer_begin_info = {
		vk::CommandBufferUsageFlagBits::eRenderPassContinue,
		&pass->cmd_buffer_inheritance_info,
	};
}

auto RenderResources::calculate_attachment_image_extent(
    RenderpassBlueprint::Attachment const &blueprint_attachment
) const -> vk::Extent3D
{
	const auto surface = vk_context->get_surface();

	switch (blueprint_attachment.size_type)
	{
	case Renderpass::SizeType::eSwapchainRelative:
	{
		auto const [width, height] = blueprint_attachment.size;
		return {
			static_cast<u32>(surface.get_framebuffer_extent().width * width),
			static_cast<u32>(surface.get_framebuffer_extent().height * height),
			1,
		};
	}

	case Renderpass::SizeType::eAbsolute:
	{
		auto const [width, height] = blueprint_attachment.size;
		return {
			static_cast<u32>(width),
			static_cast<u32>(height),
			1,
		};
	}

	case Renderpass::SizeType::eRelative:
	default: assert_fail("Invalid attachment size type"); return {};
	};
}

} // namespace BINDLESSVK_NAMESPACE
