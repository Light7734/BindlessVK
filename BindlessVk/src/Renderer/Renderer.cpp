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
	this->render_fences = other.render_fences;
	this->render_semaphores = other.render_semaphores;
	this->present_semaphores = other.present_semaphores;
	this->cmd_pools = other.cmd_pools;
	this->cmd_buffers = other.cmd_buffers;
	this->frame_index = other.frame_index;

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
	wait_for_frame_fence();

	auto const image_index = aquire_next_image();
	if (image_index == std::numeric_limits<u32>::max())
		return;

	device->vk().resetCommandPool(cmd_pools[frame_index]);
	auto const cmd = cmd_buffers[frame_index];

	cmd.beginDebugUtilsLabelEXT(graph->get_update_label());
	graph->on_update(frame_index, image_index);
	cmd.endDebugUtilsLabelEXT();

	for (auto *const pass : graph->get_passes())
		pass->on_update(frame_index, image_index);

	cmd.begin(vk::CommandBufferBeginInfo {});

	for (u32 i = 0; i < graph->get_passes().size(); ++i)
	{
		dynamic_pass_info[frame_index].color_attachments.clear();
		dynamic_pass_info[frame_index].depth_attachment = vk::RenderingAttachmentInfo {};
		apply_pass_barriers(graph->get_passes()[i], image_index);
		cmd.beginRendering(dynamic_pass_info[frame_index]);
		render_pass(graph, graph->get_passes()[i], image_index);
		cmd.endRendering();
	}

	apply_present_barriers(graph, image_index);

	submit_queue();
	present_frame(image_index);

	reset_used_attachment_states();

	frame_index = (frame_index + 1u) % max_frames_in_flight;
}

void Renderer::update_pass(Renderpass *const pass, u32 const image_index)
{
	pass->on_update(frame_index, image_index);
}

void Renderer::apply_pass_barriers(Renderpass *const pass, u32 const image_index)
{
	auto const cmd = cmd_buffers[frame_index];

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

void Renderer::apply_present_barriers(Rendergraph *const graph, u32 const image_index)
{
	auto const cmd = cmd_buffers[frame_index];
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

void Renderer::render_pass(Rendergraph *const graph, Renderpass *const pass, u32 const image_index)
{
	auto const cmd = cmd_buffers[frame_index];
	cmd.beginDebugUtilsLabelEXT(pass->get_render_label());
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    graph->get_pipeline_layout(),
	    0,
	    graph->get_descriptor_sets()[frame_index].vk(),
	    {}
	);
	auto const &descriptor_sets = pass->get_descriptor_sets();
	if (!descriptor_sets.empty())
		cmd.bindDescriptorSets(
		    vk::PipelineBindPoint::eGraphics,
		    pass->get_pipeline_layout(),
		    1,
		    descriptor_sets[frame_index].vk(),
		    {}
		);

	pass->on_render(cmd, frame_index, image_index);
	cmd.endDebugUtilsLabelEXT();
}

void Renderer::wait_for_frame_fence()
{
	assert_false(device->vk().waitForFences(render_fences[frame_index], VK_TRUE, UINT64_MAX));
	device->vk().resetFences(render_fences[frame_index]);
}

auto Renderer::aquire_next_image() -> u32
{
	auto const [result, index] = device->vk().acquireNextImageKHR(
	    swapchain.vk(),
	    std::numeric_limits<u64>::max(),
	    render_semaphores[frame_index],
	    VK_NULL_HANDLE
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

void Renderer::submit_queue()
{
	auto const graphics_queue = queues->get_graphics();
	auto const cmd = cmd_buffers[frame_index];
	cmd.end();

	auto const wait_stage =
	    vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	graphics_queue.submit(
	    vk::SubmitInfo {
	        render_semaphores[frame_index],
	        wait_stage,
	        cmd,
	        present_semaphores[frame_index],
	    },
	    render_fences[frame_index]
	);
}

void Renderer::present_frame(u32 const image_index)
{
	auto const present_queue = queues->get_present();
	auto const swapchain_array = arr<vk::SwapchainKHR, 1> { swapchain.vk() };

	try
	{
		auto const result = present_queue.presentKHR({
		    present_semaphores[frame_index],
		    swapchain_array,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || swapchain.is_invalid())
		{
			device->vk().waitIdle();
			swapchain.invalidate();
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device->vk().waitIdle();
		swapchain.invalidate();
		return;
	}
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

void Renderer::create_sync_objects()
{
	for (u32 i = 0; i < max_frames_in_flight; ++i)
	{
		render_fences[i] = device->vk().createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});
		debug_utils->set_object_name(
		    device->vk(), //
		    render_fences[i],
		    fmt::format("render_fence_{}", i)
		);

		render_semaphores[i] = device->vk().createSemaphore({});
		debug_utils->set_object_name(
		    device->vk(),
		    render_semaphores[i],
		    fmt::format("render_semaphore_{}", i)
		);

		present_semaphores[i] = device->vk().createSemaphore({});
		debug_utils->set_object_name(
		    device->vk(),
		    present_semaphores[i],
		    fmt::format("present_semaphore_{}", i)
		);
	}
}

void Renderer::destroy_sync_objects()
{
	for (u32 i = 0; i < max_frames_in_flight; i++)
	{
		device->vk().destroyFence(render_fences[i]);
		device->vk().destroySemaphore(render_semaphores[i]);
		device->vk().destroySemaphore(present_semaphores[i]);
	}
}

void Renderer::create_cmds(Gpu const *const gpu)
{
	for (u32 i = 0; i < max_frames_in_flight; i++)
	{
		cmd_pools[i] = device->vk().createCommandPool(vk::CommandPoolCreateInfo {
		    {},
		    gpu->get_graphics_queue_index(),
		});
		debug_utils->set_object_name(
		    device->vk(), //
		    cmd_pools[i],
		    fmt::format("renderer_cmd_pool_{}", i)
		);

		cmd_buffers[i] = device->vk().allocateCommandBuffers({
		    cmd_pools[i],
		    vk::CommandBufferLevel::ePrimary,
		    1u,
		})[0];
		debug_utils->set_object_name(
		    device->vk(),
		    cmd_buffers[i],
		    fmt::format("renderer_cmd_buffer_{}", i)
		);
	}
}

void Renderer::destroy_cmds()
{
	for (auto const cmd_pool : cmd_pools)
		device->vk().destroyCommandPool(cmd_pool);
}

} // namespace BINDLESSVK_NAMESPACE
