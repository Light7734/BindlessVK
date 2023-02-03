#include "BindlessVk/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(ref<VkContext const> const vk_context): vk_context(vk_context)
{
	create_sync_objects();
	create_cmd_buffers();
	on_swapchain_invalidated();
}

Renderer::~Renderer()
{
	auto const device = vk_context->get_device();

	destroy_swapchain_image_views();
	destroy_sync_objects();
	device.destroySwapchainKHR(swapchain);
}

void Renderer::render_graph(RenderGraph *const render_graph, void *const user_pointer)
{
	auto const device = vk_context->get_device();

	// Wait for frame's fence
	auto const render_fence = render_fences[current_frame];

	assert_false(device.waitForFences(render_fence, VK_TRUE, UINT64_MAX));

	device.resetFences(render_fence);

	// Aquire next swapchain image
	auto const render_semaphore = render_semaphores[current_frame];
	auto const [result, image_index] =
	    device.acquireNextImageKHR(swapchain, UINT64_MAX, render_semaphore, VK_NULL_HANDLE);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
	    is_swapchain_invalid()) [[unlikely]]
	{
		device.waitIdle();
		swapchain_invalid = true;
		return;
	}
	else
	{
		assert_false(
		    result,
		    "VkAcqtireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or "
		    "VK_SUBOPTIMAL_KHR"
		);
	}

	// Draw scene
	auto const cmd = cmd_buffers[current_frame];
	device.resetCommandPool(vk_context->get_cmd_pool(current_frame));
	render_graph->update_and_render(cmd, current_frame, image_index, user_pointer);

	// Submit & present
	auto present_semaphore = present_semaphores[current_frame];
	submit_queue(render_semaphore, present_semaphore, render_fence, cmd);
	present_frame(image_index);

	// ++
	current_frame = (current_frame + 1u) % BVK_MAX_FRAMES_IN_FLIGHT;
}

void Renderer::submit_queue(
    vk::Semaphore wait_semaphore,
    vk::Semaphore signal_semaphore,
    vk::Fence signal_fence,
    vk::CommandBuffer cmd
)
{
	auto const queues = vk_context->get_queues();

	cmd.end();

	auto const wait_stage =
	    vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	queues.graphics.submit(
	    vk::SubmitInfo {
	        wait_semaphore,
	        wait_stage,
	        cmd,
	        signal_semaphore,
	    },
	    signal_fence
	);
}

void Renderer::present_frame(u32 const image_index)
{
	auto const device = vk_context->get_device();
	auto const queues = vk_context->get_queues();

	try
	{
		auto const result = queues.present.presentKHR({
		    present_semaphores[current_frame],
		    swapchain,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || is_swapchain_invalid())
		{
			device.waitIdle();
			swapchain_invalid = true;
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device.waitIdle();
		swapchain_invalid = true;
		return;
	}
}

void Renderer::on_swapchain_invalidated()
{
	auto const device = vk_context->get_device();

	destroy_swapchain_image_views();
	create_swapchain_images();
	create_swapchain_image_views();
	device.waitIdle();

	swapchain_invalid = false;
}

void Renderer::create_swapchain_images()
{
	auto const device = vk_context->get_device();
	auto const queues = vk_context->get_queues();
	auto const surface = vk_context->get_surface();

	auto const image_count = calculate_swapchain_image_count();

	// Create swapchain
	auto const queues_have_same_index = queues.graphics_index == queues.present_index;

	auto const old_swapchain = swapchain;

	auto const swapchain_info = vk::SwapchainCreateInfoKHR {
		{},
		surface,
		image_count,
		surface.color_format,
		surface.color_space,
		surface.framebuffer_extent,
		1u,
		vk::ImageUsageFlagBits::eColorAttachment,
		queues_have_same_index ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
		queues_have_same_index ? 0u : 2u,
		queues_have_same_index ? nullptr : &queues.graphics_index,
		surface.capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		surface.present_mode,
		VK_TRUE,
		old_swapchain,
	};

	swapchain = device.createSwapchainKHR(swapchain_info, nullptr);
	device.destroySwapchainKHR(old_swapchain);

	swapchain_images = device.getSwapchainImagesKHR(swapchain);
}

void Renderer::create_swapchain_image_views()
{
	auto const device = vk_context->get_device();
	auto const queues = vk_context->get_queues();
	auto const surface = vk_context->get_surface();

	auto const image_count = swapchain_images.size();

	swapchain_image_views.resize(image_count);

	for (u32 i = 0; i < image_count; i++)
	{
		swapchain_image_views[i] = device.createImageView({
		    {},
		    swapchain_images[i],
		    vk::ImageViewType::e2D,
		    surface.color_format,
		    vk::ComponentMapping {
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		    },
		    vk::ImageSubresourceRange {
		        vk::ImageAspectFlagBits::eColor,
		        0,
		        1,
		        0,
		        1,
		    },
		});

		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (u64)(VkImage)swapchain_images[i],
		    fmt::format("swap_chain_image_{}", i).c_str(),
		});

		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (u64)(VkImageView)swapchain_image_views[i],
		    fmt::format("swapchain_image_view_{}", i).c_str(),
		});
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

void Renderer::create_cmd_buffers()
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

void Renderer::destroy_swapchain_image_views()
{
	auto const device = vk_context->get_device();
	device.waitIdle();

	for (auto const &imageView : swapchain_image_views)
		device.destroyImageView(imageView);
};

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

auto Renderer::calculate_swapchain_image_count() const -> u32
{
	auto const surface = vk_context->get_surface();

	auto const min_image_count = surface.capabilities.minImageCount;
	auto const max_image_count = surface.capabilities.maxImageCount;

	auto const has_max_limit = max_image_count != 0;

	// desired image count is in range
	if ((!has_max_limit || max_image_count >= DESIRED_SWAPCHAIN_IMAGES) &&
	    min_image_count <= DESIRED_SWAPCHAIN_IMAGES)
		return DESIRED_SWAPCHAIN_IMAGES;

	// fall-back to 2 if in ange
	else if (min_image_count <= 2 && max_image_count >= 2)
		return 2;

	// fall-back to min_image_count
	else
		return min_image_count;
}

} // namespace BINDLESSVK_NAMESPACE
