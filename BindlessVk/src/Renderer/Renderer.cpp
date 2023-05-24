#include "BindlessVk/Renderer/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(VkContext const *const vk_context, MemoryAllocator *const memory_allocator)
    : device(vk_context->get_device())
    , surface(vk_context->get_surface())
    , queues(vk_context->get_queues())
    , debug_utils(vk_context->get_debug_utils())
    , swapchain(vk_context)
    , resources(vk_context, memory_allocator, &swapchain)
{
	create_sync_objects();
	create_cmds(vk_context->get_gpu());

	image_index = acquire_next_image_index();
}

Renderer::Renderer(Renderer &&other)
{
	*this = std::move(other);
}

Renderer &Renderer::operator=(Renderer &&other)
{
	this->device = other.device;
	this->surface = other.surface;
	this->queues = other.queues;
	this->debug_utils = other.debug_utils;
	this->swapchain = std::move(other.swapchain);

	this->used_attachment_indices = std::move(other.used_attachment_indices);

	this->resources = std::move(other.resources);
	this->dynamic_pass_rendering_info = std::move(other.dynamic_pass_rendering_info);

	this->compute_fences = other.compute_fences;
	this->compute_semaphores = other.compute_semaphores;

	this->graphics_fences = other.graphics_fences;
	this->graphics_semaphores = other.graphics_semaphores;

	this->present_semaphores = other.present_semaphores;

	this->compute_cmd_pools = other.compute_cmd_pools;
	this->graphics_cmd_pools = other.graphics_cmd_pools;

	this->compute_cmds = other.compute_cmds;
	this->graphics_cmds = other.graphics_cmds;

	this->frame_index = other.frame_index;
	this->image_index = other.image_index;

	other.device = {};

	return *this;
}

Renderer::~Renderer()
{
	if (!device)
		return;

	destroy_cmds();
	destroy_sync_objects();
}

void Renderer::render_graph(RenderNode *const root_node)
{
	if (image_index == std::numeric_limits<u32>::max())
		return;

	prepare_frame(root_node);
	compute_frame(root_node);
	graphics_frame(root_node);
	present_frame();

	cycle_frame_index();
	image_index = acquire_next_image_index();
}

// 1 //
void Renderer::create_sync_objects()
{
	for (u32 i = 0; i < max_frames_in_flight; ++i)
	{
		create_compute_sync_objects(i);
		create_graphics_sync_objects(i);
		create_present_sync_objects(i);
	}
}

void Renderer::create_cmds(Gpu const *const gpu)
{
	for (u32 i = 0; i < max_frames_in_flight; ++i)
	{
		create_graphics_cmds(gpu, i);
		create_compute_cmds(gpu, i);
	}
}

void Renderer::destroy_cmds()
{
	for (auto const cmd_pool : graphics_cmd_pools)
		device->vk().destroyCommandPool(cmd_pool);
}

void Renderer::destroy_sync_objects()
{
	for (u32 i = 0; i < max_frames_in_flight; ++i)
	{
		device->vk().destroyFence(compute_fences[i]);
		device->vk().destroySemaphore(compute_semaphores[i]);

		device->vk().destroyFence(graphics_fences[i]);
		device->vk().destroySemaphore(graphics_semaphores[i]);

		device->vk().destroySemaphore(present_semaphores[i]);
	}
}

void Renderer::prepare_frame(RenderNode *const node)
{
	reset_used_attachment_states();
	prepare_node(node);
}

void Renderer::compute_frame(RenderNode *const node)
{
	wait_for_compute_fence();

	auto const cmd = compute_cmds[frame_index];
	device->vk().resetCommandPool(compute_cmd_pools[frame_index]);

	cmd.begin(vk::CommandBufferBeginInfo {});
	compute_node(node, -1);
	cmd.end();

	submit_compute_queue();
}

void Renderer::graphics_frame(RenderNode *node)
{
	wait_for_graphics_fence();

	auto const cmd = graphics_cmds[frame_index];
	device->vk().resetCommandPool(graphics_cmd_pools[frame_index]);

	cmd.begin(vk::CommandBufferBeginInfo {});
	graphics_node(node, -1);
	apply_backbuffer_barrier();
	cmd.end();

	submit_graphics_queue();
}

void Renderer::present_frame()
{
	submit_present_queue();
}

void Renderer::cycle_frame_index()
{
	frame_index = (frame_index + 1u) % max_frames_in_flight;
}

auto Renderer::acquire_next_image_index() -> u32
{
	auto const [result, index] = device->vk().acquireNextImageKHR(
	    swapchain.vk(),
	    std::numeric_limits<u64>::max(),
	    present_semaphores[frame_index],
	    {}
	);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
	    swapchain.is_invalid()) [[unlikely]]
	{
		device->vk().waitIdle();
		swapchain.invalidate();
		return std::numeric_limits<u32>::max();
	}

	assert_false(
	    result,
	    "VkAcqtireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or "
	    "VK_SUBOPTIMAL_KHR"
	);

	return index;
}

// 2 //
void Renderer::create_compute_sync_objects(u32 index)
{
	compute_fences[index] = device->vk().createFence(vk::FenceCreateInfo {
	    vk::FenceCreateFlagBits::eSignaled,
	});
	compute_semaphores[index] = device->vk().createSemaphore({});

	debug_utils->set_object_name(
	    device->vk(), //
	    compute_fences[index],
	    fmt::format("compute_fence_{}", index)
	);

	debug_utils->set_object_name(
	    device->vk(),
	    compute_semaphores[index],
	    fmt::format("compute_semaphores_{}", index)
	);
}

void Renderer::create_graphics_sync_objects(u32 index)
{
	graphics_fences[index] = device->vk().createFence(vk::FenceCreateInfo {
	    vk::FenceCreateFlagBits::eSignaled,
	});
	graphics_semaphores[index] = device->vk().createSemaphore({});

	debug_utils->set_object_name(
	    device->vk(), //
	    graphics_fences[index],
	    fmt::format("graphics_fence_{}", index)
	);

	debug_utils->set_object_name(
	    device->vk(),
	    graphics_semaphores[index],
	    fmt::format("graphics_semaphore{}", index)
	);
}

void Renderer::create_present_sync_objects(u32 index)
{
	present_semaphores[index] = device->vk().createSemaphore({});

	debug_utils->set_object_name(
	    device->vk(),
	    present_semaphores[index],
	    fmt::format("present_semaphore_{}", index)
	);
}

void Renderer::create_compute_cmds(Gpu const *const gpu, u32 index)
{
	compute_cmd_pools[index] = device->vk().createCommandPool(vk::CommandPoolCreateInfo {
	    {},
	    gpu->get_compute_queue_index(),
	});

	debug_utils->set_object_name(
	    device->vk(), // :D
	    compute_cmd_pools[index],
	    fmt::format("compute_cmd_pool_{}", index)
	);

	compute_cmds[index] = device->vk().allocateCommandBuffers({
	    compute_cmd_pools[index],
	    vk::CommandBufferLevel::ePrimary,
	    1u,
	})[0];

	debug_utils->set_object_name(
	    device->vk(),
	    compute_cmds[index],
	    fmt::format("compute_cmd_buffer_{}", index)
	);
}

void Renderer::create_graphics_cmds(Gpu const *const gpu, u32 index)
{
	graphics_cmd_pools[index] = device->vk().createCommandPool(vk::CommandPoolCreateInfo {
	    {},
	    gpu->get_graphics_queue_index(),
	});

	debug_utils->set_object_name(
	    device->vk(), // :D
	    graphics_cmd_pools[index],
	    fmt::format("graphics_cmd_pool_{}", index)
	);

	graphics_cmds[index] = device->vk().allocateCommandBuffers({
	    graphics_cmd_pools[index],
	    vk::CommandBufferLevel::ePrimary,
	    1u,
	})[0];

	debug_utils->set_object_name(
	    device->vk(),
	    graphics_cmds[index],
	    fmt::format("graphics_cmd_buffer_{}", index)
	);
}

void Renderer::prepare_node(RenderNode *const node)
{
	node->on_frame_prepare(frame_index, image_index);

	for (auto *const child_node : node->get_children())
		prepare_node(child_node);
}

void Renderer::compute_node(RenderNode *const node, i32 depth)
{
	auto const cmd = compute_cmds[frame_index];
	++depth;

	cmd.beginDebugUtilsLabelEXT(node->get_compute_label());

	bind_node_compute_descriptors(node, depth);
	node->on_frame_compute(cmd, frame_index, image_index);

	for (auto *const child_node : node->get_children())
		compute_node(child_node, depth);

	cmd.endDebugUtilsLabelEXT();
}

void Renderer::graphics_node(RenderNode *const node, i32 depth)
{
	auto const cmd = graphics_cmds[frame_index];
	++depth;

	cmd.beginDebugUtilsLabelEXT(node->get_graphics_label());

	// This call will end the dynamic renderpass if any barrier is applied
	// because barriers can't be applied during a dynamic renderpass
	auto applied_any_barriers = try_apply_node_barriers(node);

	if (applied_any_barriers)
	{
		parse_node_rendering_info(node);
		cmd.beginRendering(dynamic_pass_rendering_info.vk());
		begun_rendering = true;
	}

	bind_node_graphics_descriptors(node, depth);
	node->on_frame_graphics(cmd, frame_index, image_index);
	dynamic_pass_rendering_info.reset();

	for (auto *const child_node : node->get_children())
		graphics_node(child_node, depth);

	cmd.endDebugUtilsLabelEXT();
}

auto Renderer::try_apply_node_barriers(RenderNode *node) -> bool
{
	auto const cmd = graphics_cmds[frame_index];
	auto applied_any_barriers = false;

	for (auto const &attachment_slot : node->get_attachments())
	{
		auto *const attachment = resources.get_attachment(
		    attachment_slot.resource_index, //
		    image_index,
		    frame_index
		);

		cmd.beginDebugUtilsLabelEXT(node->get_barrier_label());

		used_attachment_indices.push_back({
		    attachment_slot.resource_index,
		    image_index,
		    frame_index,
		});

		if (!attachment_slot.is_compatible_with(attachment))
		{
			if (!applied_any_barriers)
			{
				applied_any_barriers = true;

				if (begun_rendering)
				{
					cmd.endRendering();
					begun_rendering = false;
				}
			}

			cmd.pipelineBarrier(
			    attachment->stage_mask,
			    attachment_slot.stage_mask,
			    {},
			    {},
			    {},
			    vk::ImageMemoryBarrier {
			        attachment->access_mask,
			        attachment_slot.access_mask,
			        attachment->image_layout,
			        attachment_slot.image_layout,
			        queues->get_graphics_index(),
			        queues->get_graphics_index(),
			        attachment->image.vk(),
			        attachment_slot.subresource_range,
			    }
			);

			attachment->access_mask = attachment_slot.access_mask;
			attachment->stage_mask = attachment_slot.stage_mask;
			attachment->image_layout = attachment_slot.image_layout;
		}
	}

	return applied_any_barriers;
}

void Renderer::parse_node_rendering_info(RenderNode *node)
{
	// alias
	auto &info = dynamic_pass_rendering_info;

	for (auto const &attachment_slot : node->get_attachments())
	{
		auto *const attachment_container =
		    resources.get_attachment_container(attachment_slot.resource_index);

		auto *const attachment =
		    resources.get_attachment(attachment_slot.resource_index, image_index, frame_index);

		if (attachment_slot.is_color_attachment())
		{
			auto const transient_attachment =
			    resources.get_transient_attachment(attachment_slot.transient_resource_index);

			info.color_attachments.emplace_back(vk::RenderingAttachmentInfo {
			    transient_attachment->image_view,
			    attachment->image_layout,
			    attachment_slot.transient_resolve_mode,
			    attachment->image_view,
			    attachment_slot.image_layout,
			    attachment_slot.load_op,
			    attachment_slot.store_op,
			    attachment_slot.clear_value,
			});
		}
		else
		{
			info.depth_attachment = vk::RenderingAttachmentInfo {
				attachment->image_view,
				attachment->image_layout,
				vk::ResolveModeFlagBits::eNone,
				{},
				{},
				attachment_slot.load_op,
				attachment_slot.store_op,
				attachment_slot.clear_value,
			};
		}
	}

	info.rendering_info = vk::RenderingInfo {
		{},
		vk::Rect2D {
		    { 0, 0 },
		    surface->get_framebuffer_extent(),
		},
		1,
		{},
		static_cast<u32>(info.color_attachments.size()),
		info.color_attachments.data(),
		info.depth_attachment.imageView ? &info.depth_attachment : nullptr,
		{},
	};
}

void Renderer::reset_used_attachment_states()
{
	for (auto const &[container_index, image_index, frame_index] : used_attachment_indices)
	{
		auto *const container = resources.get_attachment_container(container_index);
		auto *const attachment = container->get_attachment(image_index, frame_index);

		attachment->access_mask = {};
		attachment->image_layout = vk::ImageLayout::eUndefined;
		attachment->stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
	}
}

void Renderer::wait_for_compute_fence()
{
	assert_false(device->vk().waitForFences(
	    compute_fences[frame_index],
	    true,
	    std::numeric_limits<u32>::max()
	));

	device->vk().resetFences(compute_fences[frame_index]);
}

void Renderer::wait_for_graphics_fence()
{
	assert_false(device->vk().waitForFences(
	    graphics_fences[frame_index],
	    true,
	    std::numeric_limits<u32>::max()
	));

	device->vk().resetFences(graphics_fences[frame_index]);
}

void Renderer::bind_node_compute_descriptors(RenderNode *const node, i32 depth)
{
	auto const cmd = compute_cmds[frame_index];
	auto const &descriptor_sets = node->get_compute_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute,
	    node->get_compute_pipeline_layout(),
	    depth,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::bind_node_graphics_descriptors(RenderNode *const node, i32 depth)
{
	auto const cmd = graphics_cmds[frame_index];
	auto const &descriptor_sets = node->get_graphics_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    node->get_graphics_pipeline_layout(),
	    depth,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::apply_backbuffer_barrier()
{
	auto const cmd = graphics_cmds[frame_index];

	auto *const backbuffer_container = resources.get_attachment_container(0);
	auto *const backbuffer_attachment = backbuffer_container->get_attachment(image_index, 0);

	cmd.endRendering();
	begun_rendering = false;

	cmd.pipelineBarrier(
	    backbuffer_attachment->stage_mask,
	    vk::PipelineStageFlagBits::eBottomOfPipe,
	    {},
	    {},
	    {},
	    vk::ImageMemoryBarrier {
	        backbuffer_attachment->access_mask,
	        {},
	        backbuffer_attachment->image_layout,
	        vk::ImageLayout::ePresentSrcKHR,
	        {},
	        {},
	        backbuffer_attachment->image.vk(),
	        vk::ImageSubresourceRange {
	            vk::ImageAspectFlagBits::eColor,
	            0,
	            1,
	            0,
	            1,
	        },
	    }
	);

	backbuffer_attachment->stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
	backbuffer_attachment->image_layout = vk::ImageLayout::eUndefined;
	backbuffer_attachment->access_mask = {};

	cmd.endDebugUtilsLabelEXT();
}

void Renderer::submit_compute_queue()
{
	auto const compute_queue = queues->get_compute();
	auto const cmd = compute_cmds[frame_index];
	auto const wait_semaphores = vec<vk::Semaphore> {};
	auto const wait_stages = vec<vk::PipelineStageFlags> {};

	compute_queue.submit(
	    vk::SubmitInfo {
	        {},
	        {},
	        cmd,
	        compute_semaphores[frame_index],
	    },
	    compute_fences[frame_index]
	);
}

void Renderer::submit_graphics_queue()
{
	auto const graphics_queue = queues->get_graphics();
	auto const cmd = graphics_cmds[frame_index];

	auto const wait_semaphores = vec<vk::Semaphore> {
		compute_semaphores[frame_index],
		present_semaphores[frame_index],
	};

	auto const wait_stages = vec<vk::PipelineStageFlags> {
		vk::PipelineStageFlagBits::eDrawIndirect,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
	};

	graphics_queue.submit(
	    vk::SubmitInfo {
	        wait_semaphores,
	        wait_stages,
	        cmd,
	        graphics_semaphores[frame_index],
	    },
	    graphics_fences[frame_index]
	);
}

void Renderer::submit_present_queue()
{
	try
	{
		auto const present_queue = queues->get_present();
		auto const image = swapchain.vk();

		auto const result = present_queue.presentKHR({
		    graphics_semaphores[frame_index],
		    image,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || swapchain.is_invalid()) [[unlikely]]
		{
			device->vk().waitIdle();
			swapchain.invalidate();
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device->vk().waitIdle();
		swapchain.invalidate();
	}
}

} // namespace BINDLESSVK_NAMESPACE
