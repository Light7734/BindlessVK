#include "BindlessVk/Renderer/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(ref<VkContext> const vk_context): vk_context(vk_context), resources(vk_context)
{
	create_sync_objects();
	allocate_cmd_buffers();
}

Renderer::~Renderer()
{
	free_cmd_buffers();
	destroy_sync_objects();
}

void Renderer::render_graph(Rendergraph *const graph)
{
	wait_for_frame_fence();

	auto const device = vk_context->get_device();
	auto const image_index = aquire_next_image();
	if (image_index == std::numeric_limits<u32>::max())
		return;

	device.resetCommandPool(vk_context->get_cmd_pool(frame_index));
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

	frame_index = (frame_index + 1u) % BVK_MAX_FRAMES_IN_FLIGHT;
}

void Renderer::update_pass(Renderpass *const pass, u32 const image_index)
{
	pass->on_update(frame_index, image_index);
}

void Renderer::apply_pass_barriers(Renderpass *const pass, u32 const image_index)
{
	auto const &queues = vk_context->get_queues();
	auto const &surface = vk_context->get_surface();
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
			        queues.get_graphics_index(),
			        queues.get_graphics_index(),
			        attachment->image,
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
			        transient_attachment.image_view,
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
		    surface.get_framebuffer_extent(),
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
	        backbuffer_attachment->image,
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

void Renderer ::render_pass(Rendergraph *const graph, Renderpass *const pass, u32 const image_index)
{
	auto const cmd = cmd_buffers[frame_index];
	cmd.beginDebugUtilsLabelEXT(pass->get_render_label());
	cmd.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    graph->get_pipeline_layout(),
	    0,
	    1,
	    &graph->get_descriptor_sets()[frame_index].descriptor_set,
	    0,
	    {}
	);
	auto const &descriptor_sets = pass->get_descriptor_sets();
	if (!descriptor_sets.empty())
		cmd.bindDescriptorSets(
		    vk::PipelineBindPoint::eGraphics,
		    pass->get_pipeline_layout(),
		    1,
		    1,
		    &descriptor_sets[frame_index].descriptor_set,
		    0,
		    {}
		);

	pass->on_render(cmd, frame_index, image_index);
	cmd.endDebugUtilsLabelEXT();
}

void Renderer::wait_for_frame_fence()
{
	auto const device = vk_context->get_device();

	assert_false(device.waitForFences(render_fences[frame_index], VK_TRUE, UINT64_MAX));
	device.resetFences(render_fences[frame_index]);
}

auto Renderer::aquire_next_image() -> u32
{
	auto *const swapchain = vk_context->get_swapchain();
	auto const device = vk_context->get_device();

	auto const [result, index] = device.acquireNextImageKHR(
	    *swapchain,
	    std::numeric_limits<u64>::max(),
	    render_semaphores[frame_index],
	    VK_NULL_HANDLE
	);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
	    swapchain->is_invalid()) [[unlikely]]
	{
		device.waitIdle();
		swapchain->invalidate();
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
	auto const graphics_queue = vk_context->get_queues().get_graphics();
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
	auto const device = vk_context->get_device();
	auto const present_queue = vk_context->get_queues().get_present();
	auto *const swapchain = vk_context->get_swapchain();
	auto const swapchain_array = arr<vk::SwapchainKHR, 1> { *swapchain };
	;

	try
	{
		auto const result = present_queue.presentKHR({
		    present_semaphores[frame_index],
		    swapchain_array,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || swapchain->is_invalid())
		{
			device.waitIdle();
			swapchain->invalidate();
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device.waitIdle();
		swapchain->invalidate();
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
	auto const device = vk_context->get_device();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
	{
		render_fences[i] = device.createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});
		vk_context->set_object_name(render_fences[i], fmt::format("render_fence_{}", i));

		render_semaphores[i] = device.createSemaphore({});
		vk_context->set_object_name(render_semaphores[i], fmt::format("render_semaphore_{}", i));

		present_semaphores[i] = device.createSemaphore({});
		vk_context->set_object_name(present_semaphores[i], fmt::format("present_semaphore_{}", i));
	}
}

void Renderer::destroy_sync_objects()
{
	auto const device = vk_context->get_device();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		device.destroyFence(render_fences[i]);
		device.destroySemaphore(render_semaphores[i]);
		device.destroySemaphore(present_semaphores[i]);
	}
}

void Renderer::allocate_cmd_buffers()
{
	auto const device = vk_context->get_device();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		cmd_buffers.emplace_back(device.allocateCommandBuffers({
		    vk_context->get_cmd_pool(i),
		    vk::CommandBufferLevel::ePrimary,
		    1u,
		})[0]);

		vk_context->set_object_name(cmd_buffers[i], fmt::format("renderer_cmd_buffer_{}", i));
	}
}

void Renderer::free_cmd_buffers()
{
	auto const device = vk_context->get_device();
	auto const cmd_pool = vk_context->get_cmd_pool(0, 0);

	for (auto cmd_buffer : cmd_buffers)
		device.freeCommandBuffers(cmd_pool, cmd_buffer);

	cmd_buffers.clear();
}

} // namespace BINDLESSVK_NAMESPACE
