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
	this->dynamic_pass_info = std::move(other.dynamic_pass_info);

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

void Renderer::render_graph(Rendergraph *const graph)
{
	if (image_index == std::numeric_limits<u32>::max())
		return;

	prepare_frame(graph);

	if (graph->has_compute())
		compute_frame(graph);

	if (graph->has_graphics())
		render_frame(graph);

	present_frame(graph);

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
		device->vk().destroyFence(graphics_fences[i]);
		device->vk().destroySemaphore(compute_semaphores[i]);
		device->vk().destroySemaphore(graphics_semaphores[i]);
		device->vk().destroySemaphore(present_semaphores[i]);
	}
}

void Renderer::prepare_frame(Rendergraph *const graph)
{
	reset_used_attachment_states();

	graph->on_frame_prepare(frame_index, image_index);

	for (auto *const pass : graph->get_passes())
		pass->on_frame_prepare(frame_index, image_index);
}

void Renderer::compute_frame(Rendergraph *const graph)
{
	wait_for_compute_fence();

	auto const cmd = compute_cmds[frame_index];
	device->vk().resetCommandPool(compute_cmd_pools[frame_index]);

	cmd.begin(vk::CommandBufferBeginInfo {});
	cmd.beginDebugUtilsLabelEXT(graph->get_compute_label());

	bind_graph_compute_descriptor(graph);

	graph->on_frame_compute(cmd, frame_index, image_index);

	for (auto *const pass : graph->get_passes())
		if (pass->has_compute())
			compute_pass(graph, pass);

	cmd.end();

	submit_compute_queue();
}

void Renderer::render_frame(Rendergraph *graph)
{
	wait_for_graphics_fence();

	auto const cmd = graphics_cmds[frame_index];
	device->vk().resetCommandPool(graphics_cmd_pools[frame_index]);

	cmd.begin(vk::CommandBufferBeginInfo {});
	cmd.beginDebugUtilsLabelEXT(graph->get_graphics_label());

	bind_graph_graphics_descriptor(graph);

	graph->on_frame_graphics(cmd, frame_index, image_index);

	for (auto *const pass : graph->get_passes())
		if (pass->has_graphics())
			graphics_pass(graph, pass);


	apply_graph_barriers(graph);

	cmd.end();

	submit_graphics_queue();
}

void Renderer::present_frame(Rendergraph *graph)
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

void Renderer::bind_graph_compute_descriptor(Rendergraph *const graph)
{
	auto const cmd = compute_cmds[frame_index];
	auto const &descriptor_sets = graph->get_compute_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute,
	    graph->get_compute_pipeline_layout(),
	    0,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::bind_graph_graphics_descriptor(Rendergraph *const graph)
{
	auto const cmd = graphics_cmds[frame_index];
	auto const &descriptor_sets = graph->get_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    graph->get_pipeline_layout(),
	    0,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::apply_graph_barriers(Rendergraph *const graph)
{
	auto const cmd = graphics_cmds[frame_index];
	cmd.beginDebugUtilsLabelEXT(graph->get_present_barrier_label());

	auto *const backbuffer_container = resources.get_attachment_container(0);
	auto *const backbuffer_attachment = backbuffer_container->get_attachment(image_index, 0);

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

void Renderer::compute_pass(Rendergraph *const graph, Renderpass *const pass)
{
	auto const cmd = compute_cmds[frame_index];

	cmd.beginDebugUtilsLabelEXT(pass->get_compute_label());

	bind_graph_graphics_descriptor(graph);
	bind_pass_graphics_descriptor(pass);

	pass->on_frame_compute(compute_cmds[frame_index], frame_index, image_index);

	cmd.endDebugUtilsLabelEXT();
}

void Renderer::graphics_pass(Rendergraph *const graph, Renderpass *const pass)
{
	auto const cmd = graphics_cmds[frame_index];
	auto &dynamic_info = dynamic_pass_info[frame_index];

	cmd.beginDebugUtilsLabelEXT(pass->get_graphics_label());

	bind_graph_graphics_descriptor(graph);
	bind_pass_graphics_descriptor(pass);

	apply_pass_barriers(pass);

	cmd.beginRendering(dynamic_info.vk());
	pass->on_frame_graphics(cmd, frame_index, image_index);
	cmd.endRendering();

	dynamic_pass_info[frame_index].reset();

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
	        wait_semaphores,
	        wait_stages,
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
		present_semaphores[frame_index],
		// compute_semaphores[frame_index],
	};

	auto const wait_stages = vec<vk::PipelineStageFlags> {
		// vk::PipelineStageFlagBits::eDrawIndirect,
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

void Renderer::bind_pass_compute_descriptor(Renderpass *const pass)
{
	auto const cmd = compute_cmds[frame_index];
	auto const &descriptor_sets = pass->get_compute_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eCompute,
	    pass->get_compute_pipeline_layout(),
	    1,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::bind_pass_graphics_descriptor(Renderpass *pass)
{
	auto const cmd = graphics_cmds[frame_index];
	auto const &descriptor_sets = pass->get_compute_descriptor_sets();

	if (descriptor_sets.empty())
		return;

	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    pass->get_compute_pipeline_layout(),
	    1,
	    descriptor_sets[frame_index].vk(),
	    {}
	);
}

void Renderer::apply_pass_barriers(Renderpass *const pass)
{
	auto const cmd = graphics_cmds[frame_index];

	cmd.beginDebugUtilsLabelEXT(pass->get_barrier_label());
	for (auto const &attachment_slot : pass->get_attachments())
	{
		auto *const attachment_container =
		    resources.get_attachment_container(attachment_slot.resource_index);

		auto *const attachment =
		    resources.get_attachment(attachment_slot.resource_index, image_index, frame_index);


		used_attachment_indices.push_back({
		    attachment_slot.resource_index,
		    image_index,
		    frame_index,
		});

		if (attachment->access_mask != attachment_slot.access_mask ||
		    attachment->stage_mask != attachment_slot.stage_mask ||
		    attachment->image_layout != attachment_slot.image_layout)
		{
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

		auto rendering_attachment_info = vk::RenderingAttachmentInfo {};
		if (attachment_slot.is_color_attachment())
		{
			auto const transient_attachment =
			    resources.get_transient_attachment(attachment_slot.transient_resource_index);

			dynamic_pass_info[frame_index].color_attachments.emplace_back(
			    vk::RenderingAttachmentInfo {
			        transient_attachment->image_view,
			        attachment->image_layout,
			        attachment_slot.transient_resolve_moode,
			        attachment->image_view,
			        attachment_slot.image_layout,
			        attachment_slot.load_op,
			        attachment_slot.store_op,
			        attachment_slot.clear_value,
			    }
			);
		}
		else
		{
			dynamic_pass_info[frame_index].depth_attachment = vk::RenderingAttachmentInfo {
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
		// }
		// else
		// {
		// 	if (attachment_slot.is_color_attachment())
		// 		dynamic_pass_info[frame_index].color_attachments.emplace_back(
		// 		    vk::RenderingAttachmentInfo {
		// 		        attachment->image_view,
		// 		        attachment->image_layout,
		// 		        vk::ResolveModeFlagBits::eNone,
		// 		        {},
		// 		        {},
		// 		        attachment_slot.load_op,
		// 		        attachment_slot.store_op,
		// 		        attachment_slot.clear_value,
		// 		    }
		// 		);
		// 	else
		// 		dynamic_pass_info[frame_index].depth_attachment = vk::RenderingAttachmentInfo {
		// 			attachment->image_view,
		// 			attachment->image_layout,
		// 			vk::ResolveModeFlagBits::eNone,
		// 			{},
		// 			{},
		// 			attachment_slot.load_op,
		// 			attachment_slot.store_op,
		// 			attachment_slot.clear_value,
		// 		};
		// }
	}

	dynamic_pass_info[frame_index].rendering_info = vk::RenderingInfo {
		{},
		vk::Rect2D {
		    { 0, 0 },
		    surface->get_framebuffer_extent(),
		},
		1,
		{},
		static_cast<u32>(dynamic_pass_info[frame_index].color_attachments.size()),
		dynamic_pass_info[frame_index].color_attachments.data(),
		dynamic_pass_info[frame_index].depth_attachment.imageView ?
		    &dynamic_pass_info[frame_index].depth_attachment :
		    nullptr,
		{},
	};
	cmd.endDebugUtilsLabelEXT();
}

} // namespace BINDLESSVK_NAMESPACE
