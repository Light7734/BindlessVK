#include "BindlessVk/RenderGraph.hpp"

#include "BindlessVk/Texture.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

RenderGraph::RenderGraph(
    VkContext *vk_context,
    vec<vk::Image> swapchain_images,
    vec<vk::ImageView> swapchain_image_views
)
    : vk_context(vk_context)
    , swapchain_images(swapchain_images)
    , swapchain_image_views(swapchain_image_views)
{
}

RenderGraph::~RenderGraph()
{
	vk_context->log(LogLvl::eTrace, "Desstroying render-graph");

	const auto device = vk_context->get_device();
	const auto allocator = vk_context->get_allocator();
	const auto num_threads = vk_context->get_num_threads();

	for (auto &resource_container : attachment_resources)
	{
		if (resource_container.size_type == Renderpass::CreateInfo::SizeType::eSwapchainRelative)
		{
			//  Destroy image resources if it's not from swapchain
			if (resource_container.type != AttachmentResourceContainer::Type::ePerImage)
			{
				for (auto &resource : resource_container.resources)
				{
					device.destroyImageView(resource.image_view);
					allocator.destroyImage(resource.image, resource.image);
				}
			}
			resource_container.resources.clear();

			device.destroyImageView(resource_container.transient_ms_image_view);

			allocator.destroyImage(
			    resource_container.transient_ms_image,
			    resource_container.transient_ms_image
			);

			resource_container.transient_ms_image = {};
			resource_container.transient_ms_image_view = VK_NULL_HANDLE;
		}
	}

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		for (u32 j = 0; j < num_threads; ++j)
		{
			vk::CommandPool pool = vk_context->get_cmd_pool(i, j);

			for (u32 k = 0; k < renderpasses.size(); ++k)
			{
				device.freeCommandBuffers(pool, get_cmd(k, i, j));
			}
		}
	}

	// for (vk::CommandBuffer command_buffer : m_secondary_cmd_buffers) {
	// }

	for (auto &[key, val] : buffer_inputs)
	{
		delete val;
	}

	device.destroyPipelineLayout(pipeline_layout);
	device.destroyDescriptorSetLayout(descriptor_set_layout);

	for (auto &pass : renderpasses)
	{
		for (auto &[key, val] : pass.buffer_inputs)
		{
			delete val;
		}

		device.destroyDescriptorSetLayout(pass.descriptor_set_layout);
		device.destroyPipelineLayout(pass.pipeline_layout);
	}
}

void RenderGraph::build(
    str backbuffer_name,
    vec<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
    vec<Renderpass::CreateInfo> renderpasses,
    void (*on_update)(VkContext *, RenderGraph *, u32, void *),
    vk::DebugUtilsLabelEXT update_debug_label,
    vk::DebugUtilsLabelEXT backbuffer_barrier_debug_label
)
{
	const auto device = vk_context->get_device();

	// Assign graph members
	this->renderpasses_info = renderpasses;
	buffer_inputs_info = buffer_inputs;
	this->on_update = on_update ? on_update : [](VkContext *, RenderGraph *, u32, void *) {
	};

	swapchain_attachment_names.push_back(backbuffer_name);

	this->update_debug_label = update_debug_label;
	this->backbuffer_barrier_debug_label = backbuffer_barrier_debug_label;

	// Assign passes' members
	this->renderpasses.resize(renderpasses_info.size(), Renderpass {});
	for (u32 i = renderpasses_info.size(); i-- > 0;)
	{
		auto &pass = this->renderpasses[i];
		auto &pass_info = this->renderpasses_info[i];

		pass.name = pass_info.name;

		pass.on_update = pass_info.on_update ?
		                     pass_info.on_update :
		                     [](VkContext *, class RenderGraph *, Renderpass *, u32, void *) {
		                     };

		pass.on_render = pass_info.on_render ? pass_info.on_render :
		                                       [](VkContext *,
		                                          class RenderGraph *,
		                                          Renderpass *,
		                                          vk::CommandBuffer cmd,
		                                          u32,
		                                          u32,
		                                          void *) {
		                                       };


		pass.update_debug_label = pass_info.update_debug_label;
		pass.barrier_debug_label = pass_info.barrier_debug_label;
		pass.render_debug_label = pass_info.render_debug_label;

		for (const auto &attachment_info : this->renderpasses_info[i].color_attachments_info)
		{
			auto it = std::find(
			    swapchain_attachment_names.begin(),
			    swapchain_attachment_names.end(),
			    attachment_info.name
			);

			if (it != swapchain_attachment_names.end())
			{
				swapchain_attachment_names.push_back(attachment_info.input);
			}
		}
	}

	create_cmd_buffers();

	validate_graph();
	reorder_passes();

	build_attachment_resources();

	build_graph_texture_inputs();
	build_passes_texture_inputs();

	build_graph_buffer_inputs();
	build_passes_buffer_inputs();

	build_graph_sets();
	build_passes_sets();

	write_graph_sets();
	write_passes_sets();
	build_pass_cmd_buffer_begin_infos();
}

void RenderGraph::create_cmd_buffers()
{
	const auto device = vk_context->get_device();
	const auto num_threads = vk_context->get_num_threads();
	const auto num_pass = renderpasses_info.size();

	secondary_cmd_buffers.resize(BVK_MAX_FRAMES_IN_FLIGHT * num_threads * num_pass);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		for (u32 j = 0; j < num_threads; ++j)
		{
			const auto cmdBufferallocInfo = vk::CommandBufferAllocateInfo {
				vk_context->get_cmd_pool(i, j),
				vk::CommandBufferLevel::eSecondary,
				static_cast<u32>(num_pass),
			};

			assert_false(device.allocateCommandBuffers(
			    &cmdBufferallocInfo,
			    &secondary_cmd_buffers[num_pass * (j + (num_threads * i))]
			));
		}
	}
}

// @todo: Implement
void RenderGraph::validate_graph()
{
	vk_context->log(LogLvl::eWarn, "Unimplemented function call");
}

// @todo: Implement
void RenderGraph::reorder_passes()
{
}

void RenderGraph::build_attachment_resources()
{
	for (u32 i = 0; i < renderpasses_info.size(); ++i)
	{
		auto &pass = renderpasses[i];
		const auto &pass_info = renderpasses_info[i];

		for (u32 j = 0; j < pass_info.color_attachments_info.size(); ++j)
		{
			const auto &color_attachment_info = pass_info.color_attachments_info[j];

			// Write only ; create new resource
			if (color_attachment_info.input == "")
			{
				// Read/writes to a backbuffer resource?
				auto it = std::find(
				    swapchain_attachment_names.begin(),
				    swapchain_attachment_names.end(),
				    color_attachment_info.input
				);

				create_attachment_resource(
				    color_attachment_info,
				    it != swapchain_attachment_names.end() ?
				        AttachmentResourceContainer::Type::ePerImage :
				        AttachmentResourceContainer::Type::eSingle,
				    UINT32_MAX
				);

				pass.attachments.push_back({
				    vk::PipelineStageFlagBits::eColorAttachmentOutput,
				    vk::AccessFlagBits::eColorAttachmentWrite,
				    vk::ImageLayout::eColorAttachmentOptimal,
				    { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
				    vk::AttachmentLoadOp::eClear,
				    vk::AttachmentStoreOp::eStore,
				    static_cast<u32>(attachment_resources.size() - 1),
				    color_attachment_info.clear_value,
				});
			}
			// Read-Write ; use existing resource
			else
			{
				for (u32 k = 0; k < attachment_resources.size(); ++k)
				{
					auto &resource_container = attachment_resources[k];
					if (resource_container.last_write_name != color_attachment_info.input)
						continue;

					assert_true(
					    resource_container.size == color_attachment_info.size &&
					        resource_container.size_type == color_attachment_info.size_type,
					    "ReadWrite attachment with different size from input is currently not "
					    "supported"
					);

					resource_container.last_write_name = color_attachment_info.name;
					pass.attachments.push_back({
					    vk::PipelineStageFlagBits::eColorAttachmentOutput,
					    vk::AccessFlagBits::eColorAttachmentWrite,
					    vk::ImageLayout::eColorAttachmentOptimal,
					    vk::ImageSubresourceRange {
					        vk::ImageAspectFlagBits::eColor,
					        0,
					        1,
					        0,
					        1,
					    },
					    vk::AttachmentLoadOp::eLoad,
					    vk::AttachmentStoreOp::eStore,
					    k,
					});

					break;
				}
			}
		}

		if (pass_info.depth_stencil_attachment_info.name.empty())
			continue;

		const auto &depth_stencil_attachment_info = pass_info.depth_stencil_attachment_info;
		// Write only ; create new resource
		if (depth_stencil_attachment_info.input == "")
		{
			auto it = std::find(
			    swapchain_attachment_names.begin(),
			    swapchain_attachment_names.end(),
			    depth_stencil_attachment_info.input
			);


			create_attachment_resource(
			    depth_stencil_attachment_info,
			    AttachmentResourceContainer::Type::eSingle,
			    UINT32_MAX
			);

			pass.attachments.push_back({
			    vk::PipelineStageFlagBits::eEarlyFragmentTests,
			    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
			        vk::AccessFlagBits::eDepthStencilAttachmentRead,
			    vk::ImageLayout::eDepthAttachmentOptimal,
			    vk::ImageSubresourceRange {
			        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
			        0,
			        1,
			        0,
			        1,
			    },
			    vk::AttachmentLoadOp::eClear,
			    vk::AttachmentStoreOp::eStore,
			    static_cast<u32>(attachment_resources.size() - 1u),
			    depth_stencil_attachment_info.clear_value,
			});
		}
		// Read-Write ; use existing resource
		else
		{
			for (u32 k = 0; k < attachment_resources.size(); ++k)
			{
				auto &resource_container = attachment_resources[k];
				if (resource_container.last_write_name == depth_stencil_attachment_info.input)
					continue;

				assert_true(
				    resource_container.size != depth_stencil_attachment_info.size &&
				        resource_container.size_type != depth_stencil_attachment_info.size_type,
				    "ReadWrite attachment with different size from input is currently not "
				    "supported"
				);

				resource_container.last_write_name = depth_stencil_attachment_info.name;
				pass.attachments.push_back({
				    vk::PipelineStageFlagBits::eEarlyFragmentTests,
				    vk::AccessFlagBits::eDepthStencilAttachmentWrite |
				        vk::AccessFlagBits::eDepthStencilAttachmentRead,
				    vk::ImageLayout::eDepthAttachmentOptimal,
				    vk::ImageSubresourceRange {
				        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
				        0,
				        1,
				        0,
				        1,
				    },
				    vk::AttachmentLoadOp::eLoad,
				    vk::AttachmentStoreOp::eStore,
				    1,
				});

				break;
			}
		}
	}
}

void RenderGraph::build_graph_texture_inputs()
{
}

void RenderGraph::build_passes_texture_inputs()
{
}

void RenderGraph::build_graph_buffer_inputs()
{
	for (auto &buffer_input_info : buffer_inputs_info)
	{
		buffer_inputs.emplace(
		    hash_str(buffer_input_info.name.c_str()),
		    new Buffer(
		        buffer_input_info.name.c_str(),
		        vk_context,
		        buffer_input_info.type == vk::DescriptorType::eUniformBuffer ?
		            vk::BufferUsageFlagBits::eUniformBuffer :
		            vk::BufferUsageFlagBits::eStorageBuffer,
		        vma::AllocationCreateInfo {
		            vma::AllocationCreateFlagBits::eHostAccessRandom,
		            vma::MemoryUsage::eAutoPreferDevice,
		        },
		        buffer_input_info.size,
		        BVK_MAX_FRAMES_IN_FLIGHT
		    )
		);
	}
}

void RenderGraph::build_passes_buffer_inputs()
{
	for (u32 i = 0; i < renderpasses_info.size(); ++i)
	{
		auto &pass = renderpasses[i];
		const auto &pass_info = renderpasses_info[i];

		for (auto &buffer_input_info : pass_info.buffer_inputs_info)
		{
			pass.buffer_inputs.emplace(
			    hash_str(buffer_input_info.name.c_str()),
			    new Buffer(
			        buffer_input_info.name.c_str(),
			        vk_context,
			        buffer_input_info.type == vk::DescriptorType::eUniformBuffer ?
			            vk::BufferUsageFlagBits::eUniformBuffer :
			            vk::BufferUsageFlagBits::eStorageBuffer,
			        vma::AllocationCreateInfo {
			            vma::AllocationCreateFlagBits::eHostAccessRandom,
			            vma::MemoryUsage::eAutoPreferDevice,
			        },
			        buffer_input_info.size,
			        BVK_MAX_FRAMES_IN_FLIGHT
			    )
			);
		}
	}
}

void RenderGraph::build_graph_sets()
{
	auto const device = vk_context->get_device();
	auto *descriptor_allocator = vk_context->get_descriptor_allocator();

	// Create set & pipeline layout
	vec<vk::DescriptorSetLayoutBinding> bindings;
	for (const auto &buffer_input_info : buffer_inputs_info)
	{
		bindings.push_back(vk::DescriptorSetLayoutBinding {
		    buffer_input_info.binding,
		    buffer_input_info.type,
		    buffer_input_info.count,
		    buffer_input_info.stage_mask,
		});
	}
	descriptor_set_layout = device.createDescriptorSetLayout({
	    {},
	    static_cast<u32>(bindings.size()),
	    bindings.data(),
	});
	pipeline_layout = device.createPipelineLayout({ {}, 1, &descriptor_set_layout });

	// Allocate sets
	sets.resize(BVK_MAX_FRAMES_IN_FLIGHT);
	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		sets.push_back(descriptor_allocator->allocate_descriptor_set(&descriptor_set_layout));
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eDescriptorSet,
		    (uint64_t)(VkDescriptorSet)sets.back(),
		    fmt::format("render_graph_descriptor_set_{}", i).c_str(),
		});
	}
}

void RenderGraph::build_passes_sets()
{
	const auto device = vk_context->get_device();
	auto *descriptor_allocator = vk_context->get_descriptor_allocator();

	for (u32 i = 0; i < renderpasses_info.size(); ++i)
	{
		auto &pass = renderpasses[i];
		const auto &pass_info = renderpasses_info[i];

		// Create set & pipeline layout
		vec<vk::DescriptorSetLayoutBinding> bindings;
		for (const auto &buffer_input_info : pass_info.buffer_inputs_info)
		{
			bindings.push_back(vk::DescriptorSetLayoutBinding {
			    buffer_input_info.binding,
			    buffer_input_info.type,
			    buffer_input_info.count,
			    buffer_input_info.stage_mask,
			});
		}
		for (const auto &texture_input_info : pass_info.texture_inputs_info)
		{
			bindings.push_back(vk::DescriptorSetLayoutBinding {
			    texture_input_info.binding,
			    texture_input_info.type,
			    texture_input_info.count,
			    texture_input_info.stage_mask,
			});
		}
		pass.descriptor_set_layout = device.createDescriptorSetLayout(
		    { {}, static_cast<u32>(bindings.size()), bindings.data() }
		);
		arr<vk::DescriptorSetLayout, 2> layouts {
			descriptor_set_layout,
			pass.descriptor_set_layout,
		};
		pass.pipeline_layout = device.createPipelineLayout({ {}, 2ull, layouts.data() });

		// Allocate descriptor sets
		if (!pass_info.buffer_inputs_info.empty() || !pass_info.texture_inputs_info.empty())
		{
			for (u32 j = 0; j < BVK_MAX_FRAMES_IN_FLIGHT; ++j)
			{
				pass.descriptor_sets.push_back(
				    descriptor_allocator->allocate_descriptor_set(&pass.descriptor_set_layout)
				);

				device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
				    vk::ObjectType::eDescriptorSet,
				    (uint64_t)(VkDescriptorSet)pass.descriptor_sets.back(),
				    fmt::format("{}_descriptor_set_{}", pass.name, i).c_str(),
				});
			}
		}
	}
}

void RenderGraph::write_graph_sets()
{
	const auto device = vk_context->get_device();

	vec<vk::DescriptorBufferInfo> buffers_info;
	buffers_info.reserve(buffer_inputs.size() * BVK_MAX_FRAMES_IN_FLIGHT);
	for (const auto &buffer_input_info : buffer_inputs_info)
	{
		vec<vk::WriteDescriptorSet> writes;

		for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			buffers_info.push_back({
			    *(buffer_inputs[hash_str(buffer_input_info.name.c_str())]->get_buffer()),
			    buffer_inputs[hash_str(buffer_input_info.name.c_str())]->get_block_size() * i,
			    buffer_inputs[hash_str(buffer_input_info.name.c_str())]->get_block_size(),
			});

			for (u32 j = 0; j < buffer_input_info.count; ++j)
			{
				writes.push_back(vk::WriteDescriptorSet {
				    sets[i],
				    buffer_input_info.binding,
				    j,
				    1u,
				    buffer_input_info.type,
				    nullptr,
				    &buffers_info.back(),
				});
			}
		}
		device.updateDescriptorSets(writes.size(), writes.data(), 0u, nullptr);

		// @todo should we wait idle?
		device.waitIdle();
	}
}

void RenderGraph::write_passes_sets()
{
	const auto device = vk_context->get_device();

	for (u32 i = 0; i < renderpasses_info.size(); ++i)
	{
		const auto &pass_info = renderpasses_info[i];
		auto &pass = renderpasses[i];

		vec<vk::WriteDescriptorSet> set_writes;
		set_writes.reserve(pass.buffer_inputs.size() * BVK_MAX_FRAMES_IN_FLIGHT);

		vec<vk::DescriptorBufferInfo> buffer_infos;
		buffer_infos.reserve(pass.buffer_inputs.size() * BVK_MAX_FRAMES_IN_FLIGHT);

		for (const auto &buffer_input_info : pass_info.buffer_inputs_info)
		{
			for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
			{
				buffer_infos.push_back({
				    *(pass.buffer_inputs[hash_str(buffer_input_info.name.c_str())]->get_buffer()),
				    buffer_input_info.size * i,
				    buffer_input_info.size,
				});

				for (u32 j = 0; j < buffer_input_info.count; ++j)
				{
					set_writes.push_back(vk::WriteDescriptorSet {
					    pass.descriptor_sets[i],
					    buffer_input_info.binding,
					    j,
					    1u,
					    buffer_input_info.type,
					    nullptr,
					    &buffer_infos.back(),
					});
				}
			}
		}
		for (const auto &texture_input_info : pass_info.texture_inputs_info)
		{
			vk_context->log(
			    LogLvl::eWarn,
			    "{} - {}",
			    BVK_MAX_FRAMES_IN_FLIGHT,
			    pass.descriptor_sets.size()
			);

			for (u32 j = 0; j < BVK_MAX_FRAMES_IN_FLIGHT; ++j)
			{
				for (u32 k = 0; k < texture_input_info.count; ++k)
				{
					set_writes.push_back(vk::WriteDescriptorSet {
					    pass.descriptor_sets[j],
					    texture_input_info.binding,
					    k,
					    1u,
					    texture_input_info.type,
					    &texture_input_info.default_texture->descriptor_info,
					    nullptr,
					});
				}
			}
		}

		device.updateDescriptorSets(set_writes.size(), set_writes.data(), 0u, nullptr);

		// @todo should we wait idle?
		device.waitIdle();
	}
}


void RenderGraph::build_pass_cmd_buffer_begin_infos()
{
	const auto surface = vk_context->get_surface();

	for (u32 i = 0; i < renderpasses_info.size(); ++i)
	{
		auto &pass = renderpasses[i];

		for (auto &attachment : pass.attachments)
		{
			if (attachment.subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor)
			{
				pass.color_attachments_format.push_back(surface.color_format);
			}
			else
			{
				pass.depth_attachment_format = vk_context->get_depth_format();
			}
		}

		pass.cmd_buffer_inheritance_rendering_info = {
			{},
			{},
			(u32)pass.color_attachments_format.size(),
			pass.color_attachments_format.data(),
			pass.depth_attachment_format,
			{},
			sample_count,
			{},
		};

		pass.cmd_buffer_inheritance_info = {
			{},
			{},
			{},
			{},
			{},
			{}, // :D
			&pass.cmd_buffer_inheritance_rendering_info,
		};

		pass.cmd_buffer_begin_info = {
			vk::CommandBufferUsageFlagBits::eRenderPassContinue,
			&pass.cmd_buffer_inheritance_info,
		};
	}
}

void RenderGraph::create_attachment_resource(
    const Renderpass::CreateInfo::AttachmentInfo &attachment_info,
    RenderGraph::AttachmentResourceContainer::Type attachment_type,
    u32 recreate_resource_index
)
{
	const auto device = vk_context->get_device();
	const auto surface = vk_context->get_surface();
	const auto allocator = vk_context->get_allocator();

	if (recreate_resource_index == UINT32_MAX)
	{
		attachment_resources.push_back({});
	}


	sample_count = attachment_info.samples;

	// Set usage & askect masks
	vk::ImageUsageFlags image_usage_mask;
	vk::ImageAspectFlags image_aspect_mask;
	if (attachment_info.format == surface.color_format)
	{
		image_usage_mask = vk::ImageUsageFlagBits::eColorAttachment;
		image_aspect_mask = vk::ImageAspectFlagBits::eColor;
	}
	else if (attachment_info.format == vk_context->get_depth_format())
	{
		image_usage_mask = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		image_aspect_mask = vk::ImageAspectFlagBits::eDepth;
	}
	assert_true(
	    !!image_usage_mask && !!image_aspect_mask,
	    "Unsupported render attachment format: {}",
	    string_VkFormat(static_cast<VkFormat>(attachment_info.format))
	);

	// Set image extent
	vk::Extent3D image_extent = calculate_attachment_image_extent(attachment_info);

	auto *resource_container = recreate_resource_index == UINT32_MAX ?
	                             &attachment_resources.back() :
	                             &attachment_resources[recreate_resource_index];
	*resource_container = {
		.type = attachment_type,
		.image_format = attachment_info.format,

		.extent = image_extent,
		.size = attachment_info.size,
		.size_type = attachment_info.size_type,
		.relative_size_name = attachment_info.size_relative_name,

		.sample_count = attachment_info.samples,
		.transient_ms_resolve_mode = vk::ResolveModeFlagBits::eNone,
		.transient_ms_image = {},
		.transient_ms_image_view = {},
		.last_write_name = attachment_info.name,
		.cached_renderpass_info = attachment_info,
	};

	switch (attachment_type)
	{
	case AttachmentResourceContainer::Type::ePerImage:
	{
		for (u32 i = 0; i < swapchain_images.size(); ++i)
		{
			resource_container->resources.push_back(AttachmentResource {
			    .src_access_mask = {},
			    .src_image_layout = vk::ImageLayout::eUndefined,
			    .src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe,
			    .image = swapchain_images[i],
			    .image_view = swapchain_image_views[i],
			});
		}

		if (recreate_resource_index == UINT32_MAX)
		{
			swapchain_resource_index = attachment_resources.size() - 1;
		}
		break;
	}
	case AttachmentResourceContainer::Type::eSingle:
	{
		vk::ImageCreateInfo image_info {
			{},
			vk::ImageType::e2D,
			attachment_info.format,
			resource_container->extent,
			1u,
			1u,
			image_aspect_mask & vk::ImageAspectFlagBits::eColor ? vk::SampleCountFlagBits::e1 :
			                                                      attachment_info.samples,
			vk::ImageTiling::eOptimal,
			image_usage_mask,
			vk::SharingMode::eExclusive,
			0u,
			nullptr,
			vk::ImageLayout::eUndefined,
		};

		vma::AllocationCreateInfo image_allocate_info(
		    {},
		    vma::MemoryUsage::eGpuOnly,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		AllocatedImage image = allocator.createImage(image_info, image_allocate_info);
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)(vk::Image)image,
		    fmt::format("{}_image (single)", attachment_info.name).c_str(),
		});

		vk::ImageViewCreateInfo image_view_info {
			{},
			image,
			vk::ImageViewType::e2D,
			attachment_info.format,
			vk::ComponentMapping {
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			},
			vk::ImageSubresourceRange {
			    image_aspect_mask,
			    0u,
			    1u,
			    0u,
			    1u,
			},
		};
		vk::ImageView image_view = device.createImageView(image_view_info, nullptr);
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (uint64_t)(VkImageView)image_view,
		    fmt::format("{}_image_view (single)", attachment_info.name).c_str(),
		});

		resource_container->resources.push_back(AttachmentResource {
		    .src_access_mask = {},
		    .src_image_layout = vk::ImageLayout::eUndefined,
		    .src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe,
		    .image = image,
		    .image_view = image_view,
		});

		break;
	}
	default: assert_fail("Invalid attachment resource type");
	}

	if (static_cast<u32>(attachment_info.samples) > 1 &&
	    image_aspect_mask & vk::ImageAspectFlagBits::eColor)
	{
		vk::ImageCreateInfo image_create_info {
			{},
			vk::ImageType::e2D,
			attachment_info.format,
			resource_container->extent,
			1u,
			1u,
			attachment_info.samples,
			vk::ImageTiling::eOptimal,

			image_usage_mask | vk::ImageUsageFlagBits::eTransientAttachment,

			vk::SharingMode::eExclusive,
			0u,
			nullptr,
			vk::ImageLayout::eUndefined,
		};

		vma::AllocationCreateInfo image_allocate_info(
		    {},
		    vma::MemoryUsage::eGpuOnly,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		AllocatedImage image = allocator.createImage(image_create_info, image_allocate_info);

		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (uint64_t)(VkImage)(vk::Image)image,
		    fmt::format("{}_transient_ms_image", attachment_info.name).c_str(),
		});


		vk::ImageViewCreateInfo image_view_info {
			{},
			image,
			vk::ImageViewType::e2D,
			attachment_info.format,
			vk::ComponentMapping {
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			    vk::ComponentSwizzle::eIdentity,
			},
			vk::ImageSubresourceRange {
			    image_aspect_mask,
			    0u,
			    1u,
			    0u,
			    1u,
			},
		};
		vk::ImageView image_view = device.createImageView(image_view_info, nullptr);
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (uint64_t)(VkImageView)image_view,
		    fmt::format("{}_transient_ms_image_view", attachment_info.name).c_str(),
		});

		resource_container->transient_ms_image = image;
		resource_container->transient_ms_image_view = image_view;
		resource_container->transient_ms_resolve_mode = vk::ResolveModeFlagBits::eAverage;
	}
}

void RenderGraph::on_swapchain_invalidated(
    vec<vk::Image> swapchain_images,
    vec<vk::ImageView> swapchain_image_views
)
{
	const auto device = vk_context->get_device();
	const auto allocator = vk_context->get_allocator();

	swapchain_images = swapchain_images;
	swapchain_image_views = swapchain_image_views;

	for (u32 i = 0; i < attachment_resources.size(); ++i)
	{
		auto &resource_container = attachment_resources[i];
		if (resource_container.size_type == Renderpass::CreateInfo::SizeType::eSwapchainRelative)
		{
			// Destroy image resources if it's not from swapchain
			if (resource_container.type != AttachmentResourceContainer::Type::ePerImage)
			{
				for (AttachmentResource &resource : resource_container.resources)
				{
					device.destroyImageView(resource.image_view);
					allocator.destroyImage(resource.image, resource.image);
				}
			}
			resource_container.resources.clear();

			device.destroyImageView(resource_container.transient_ms_image_view);
			allocator.destroyImage(
			    resource_container.transient_ms_image,
			    resource_container.transient_ms_image
			);

			resource_container.transient_ms_image = {};
			resource_container.transient_ms_image_view = VK_NULL_HANDLE;

			create_attachment_resource(
			    resource_container.cached_renderpass_info,
			    resource_container.type,
			    i
			);
		}
	}
}

// @todo: Setup render barriers
// @todo: Multi-threaded recording
void RenderGraph::update_and_render(
    vk::CommandBuffer const primary_cmd,
    u32 const frame_index,
    u32 const image_index,
    void *const user_pointer
)
{
	const auto &queues = vk_context->get_queues();
	const auto num_threads = vk_context->get_num_threads();
	const auto thread_index = 0;

	// Update graph
	{
		queues.graphics.beginDebugUtilsLabelEXT(update_debug_label);
		on_update(vk_context, this, frame_index, user_pointer);
		queues.graphics.endDebugUtilsLabelEXT();
	}

	// Update passes
	for (u32 i = 0; i < renderpasses.size(); ++i)
	{
		auto &pass = renderpasses[i];

		queues.graphics.beginDebugUtilsLabelEXT(pass.update_debug_label);
		pass.on_update(vk_context, this, &renderpasses[i], frame_index, user_pointer);
		queues.graphics.endDebugUtilsLabelEXT(); // pass.update_debug_label
	}

	// Render passes (record their rendering cmds to secondary cmd buffers)
	primary_cmd.begin(vk::CommandBufferBeginInfo {});
	for (u32 i = 0; i < renderpasses.size(); ++i)
	{
		auto pass_cmd = secondary_cmd_buffers[(frame_index * renderpasses.size()) + i];
		record_pass_cmds(pass_cmd, frame_index, image_index, i, user_pointer);
	}

	for (u32 i = 0; i < renderpasses.size(); ++i)
	{
		auto &pass = renderpasses[i];

		auto pass_rendering_info = apply_pass_barriers(primary_cmd, frame_index, image_index, i);

		primary_cmd.beginDebugUtilsLabelEXT(pass.render_debug_label);

		primary_cmd.beginRendering(pass_rendering_info.rendering_info);
		primary_cmd.executeCommands(
		    secondary_cmd_buffers
		        [i + renderpasses.size() * (thread_index + num_threads * frame_index)]
		);
		primary_cmd.endRendering();

		primary_cmd.endDebugUtilsLabelEXT(); // pass.render_debug_label
	}

	apply_backbuffer_barrier(primary_cmd, frame_index, image_index);
}

void RenderGraph::record_pass_cmds(
    vk::CommandBuffer cmd,
    u32 frame_index,
    u32 image_index,
    u32 pass_index,
    void *user_pointer
)
{
	auto &pass = renderpasses[pass_index];
	cmd.begin(pass.cmd_buffer_begin_info);

	// Graph descriptor set (set = 0)
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    pipeline_layout,
	    0u,
	    1u,
	    &sets[frame_index],
	    0u,
	    {}
	);

	// Pass descriptor set (set = 1)
	if (!pass.descriptor_sets.empty())
	{
		cmd.bindDescriptorSets(
		    vk::PipelineBindPoint::eGraphics,
		    pass.pipeline_layout,
		    1u,
		    1u,
		    &pass.descriptor_sets[frame_index],
		    0u,
		    {}
		);
	}

	pass.on_render(
	    vk_context,
	    this,
	    &renderpasses[pass_index],
	    cmd,
	    frame_index,
	    image_index,
	    user_pointer
	);

	cmd.end();
}

RenderGraph::PassRenderingInfo RenderGraph::apply_pass_barriers(
    vk::CommandBuffer cmd,
    u32 frame_index,
    u32 image_index,
    u32 pass_index
)
{
	const auto &queues = vk_context->get_queues();
	const auto &surface = vk_context->get_surface();

	auto &pass = renderpasses[pass_index];
	cmd.beginDebugUtilsLabelEXT(pass.barrier_debug_label);

	PassRenderingInfo pass_rendering_info = {};
	for (auto &attachment : pass.attachments)
	{
		auto &resource_container = attachment_resources[attachment.resource_index];

		auto &resource = resource_container.get_resource(image_index, frame_index);

		vk::ImageMemoryBarrier image_barrier {
			resource.src_access_mask, attachment.access_mask,       resource.src_image_layout,
			attachment.layout,        queues.graphics_index,        queues.graphics_index,
			resource.image,           attachment.subresource_range, nullptr,
		};

		if ((resource.src_access_mask != attachment.access_mask ||
		     resource.src_image_layout != attachment.layout ||
		     resource.src_stage_mask != attachment.stage_mask) &&
		    attachment.subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor)
		{
			cmd.pipelineBarrier(
			    resource.src_stage_mask,
			    attachment.stage_mask,
			    {},
			    {},
			    {},
			    image_barrier
			);

			resource.src_access_mask = attachment.access_mask;
			resource.src_image_layout = attachment.layout;
			resource.src_stage_mask = attachment.stage_mask;
		}


		vk::RenderingAttachmentInfo rendering_attachment_info {};
		// Multi-sampled image
		if (static_cast<u32>(resource_container.sample_count) > 1 &&
		    attachment.subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor)
		{
			rendering_attachment_info = {
				resource_container.transient_ms_image_view,
				attachment.layout,
				resource_container.transient_ms_resolve_mode,
				resource.image_view,
				attachment.layout,
				attachment.load_op,
				attachment.store_op,
				attachment.clear_value,
			};
		}
		// Single-sampled image
		else
		{
			rendering_attachment_info = {
				resource.image_view,
				attachment.layout,
				vk::ResolveModeFlagBits::eNone,
				{},
				{}, // :D
				attachment.load_op,
				attachment.store_op,
				attachment.clear_value,
			};
		}

		if (attachment.subresource_range.aspectMask & vk::ImageAspectFlagBits::eDepth)
		{
			pass_rendering_info.depth_attachment_info = rendering_attachment_info;
		}
		else
		{
			pass_rendering_info.color_attachments_info.push_back(rendering_attachment_info);
		}
	}

	pass_rendering_info.rendering_info = vk::RenderingInfo {
		vk::RenderingFlagBits::eContentsSecondaryCommandBuffers,
		vk::Rect2D {
		    { 0, 0 },
		    surface.framebuffer_extent,
		},
		1u,
		{},
		static_cast<u32>(pass_rendering_info.color_attachments_info.size()),
		pass_rendering_info.color_attachments_info.data(),
		pass_rendering_info.depth_attachment_info.imageView ?
		    &pass_rendering_info.depth_attachment_info :
		    nullptr,
		{},
	};

	cmd.endDebugUtilsLabelEXT(); // pass.barrier_debug_label
	return pass_rendering_info;
}

void RenderGraph::apply_backbuffer_barrier(vk::CommandBuffer cmd, u32 frame_index, u32 image_index)
{
	vk::DebugUtilsLabelEXT a;
	cmd.beginDebugUtilsLabelEXT(backbuffer_barrier_debug_label);
	auto &backbuffer_resource =
	    attachment_resources[swapchain_resource_index].get_resource(image_index, frame_index);

	cmd.pipelineBarrier(
	    backbuffer_resource.src_stage_mask,
	    vk::PipelineStageFlagBits::eBottomOfPipe,
	    {},
	    {},
	    {},
	    vk::ImageMemoryBarrier {
	        backbuffer_resource.src_access_mask,
	        {},
	        backbuffer_resource.src_image_layout,
	        vk::ImageLayout::ePresentSrcKHR,
	        {},
	        {},
	        backbuffer_resource.image,
	        vk::ImageSubresourceRange {
	            vk::ImageAspectFlagBits::eColor,
	            0u,
	            1u,
	            0u,
	            1u,
	        },
	    }

	);

	backbuffer_resource.src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
	backbuffer_resource.src_image_layout = vk::ImageLayout::eUndefined;
	backbuffer_resource.src_access_mask = {};

	cmd.endDebugUtilsLabelEXT();
}

vk::Extent3D RenderGraph::calculate_attachment_image_extent(
    const Renderpass::CreateInfo::AttachmentInfo &attachment_info
)
{
	const auto surface = vk_context->get_surface();

	switch (attachment_info.size_type)
	{
	case Renderpass::CreateInfo::SizeType::eSwapchainRelative:
	{
		return {
			static_cast<u32>(surface.framebuffer_extent.width * attachment_info.size.x),
			static_cast<u32>(surface.framebuffer_extent.height * attachment_info.size.y),
			1,
		};
	}

	case Renderpass::CreateInfo::SizeType::eAbsolute:
		return {
			static_cast<u32>(attachment_info.size.x),
			static_cast<u32>(attachment_info.size.y),
			1,
		};

	case Renderpass::CreateInfo::SizeType::eRelative:
	default: assert_fail("Invalid/Unsupported attachment size type"); return {};
	};
}

} // namespace BINDLESSVK_NAMESPACE
