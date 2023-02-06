#include "BindlessVk/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(ref<VkContext> const vk_context): vk_context(vk_context)
{
	create_sync_objects();
	allocate_cmd_buffers();
}

Renderer::~Renderer()
{
	free_cmd_buffers();
	destroy_sync_objects();
}

void Renderer::render_graph(RenderGraph *const render_graph, void *const user_pointer)
{
	auto const device = vk_context->get_device();

	auto const [success, image_index] = aquire_next_image();
	if (!success)
		return;

	device.resetCommandPool(vk_context->get_cmd_pool(current_frame));

	render_graph->update(current_frame, user_pointer);

	render_graph->render(cmd_buffers[current_frame], current_frame, image_index, user_pointer);

	submit_queue();
	present_frame(image_index);

	current_frame = (current_frame + 1u) % BVK_MAX_FRAMES_IN_FLIGHT;
}

void Renderer::wait_for_frame_fence()
{
	auto const device = vk_context->get_device();

	assert_false(device.waitForFences(render_fences[current_frame], VK_TRUE, UINT64_MAX));
	device.resetFences(render_fences[current_frame]);
}

auto Renderer::aquire_next_image() -> pair<bool, u32>
{
	auto *const swapchain = vk_context->get_swapchain();
	auto const device = vk_context->get_device();

	auto const [result, index] = device.acquireNextImageKHR(
	    *swapchain,
	    std::numeric_limits<u64>::max(),
	    render_semaphores[current_frame],
	    VK_NULL_HANDLE
	);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
	    swapchain->is_invalid()) [[unlikely]]
	{
		device.waitIdle();
		swapchain->invalidate();
		return { false, 0 };
	}

	assert_false(
	    result,
	    "VkAcqtireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or "
	    "VK_SUBOPTIMAL_KHR"
	);

	return { true, index };
}

void Renderer::submit_queue()
{
	auto const graphics_queue = vk_context->get_queues().get_graphics();
	auto const cmd = cmd_buffers[current_frame];

	cmd.end();

	auto const wait_stage =
	    vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	graphics_queue.submit(
	    vk::SubmitInfo {
	        render_semaphores[current_frame],
	        wait_stage,
	        cmd,
	        present_semaphores[current_frame],
	    },
	    render_fences[current_frame]
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
		    present_semaphores[current_frame],
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

void Renderer::create_sync_objects()
{
	auto const device = vk_context->get_device();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		render_fences[i] = device.createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});

		render_semaphores[i] = device.createSemaphore({});
		present_semaphores[i] = device.createSemaphore({});
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
	}
}

void Renderer::free_cmd_buffers()
{
	auto const device = vk_context->get_device();
	auto const cmd_pool = vk_context->get_cmd_pool(0, 0);

	for (auto cmd_buffer : cmd_buffers)
		device.freeCommandBuffers(cmd_pool, cmd_buffer);
}

} // namespace BINDLESSVK_NAMESPACE
