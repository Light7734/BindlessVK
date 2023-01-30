#include "BindlessVk/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(VkContext *vk_context): vk_context(vk_context)
{
	create_sync_objects();
	create_cmd_buffers();
	on_swapchain_invalidated();
}

Renderer::Renderer(Renderer &&other)
{
	*this = std::move(other);
}

Renderer &Renderer::operator=(Renderer &&other)
{
	if (this == &other)
		return *this;

	this->vk_context = other.vk_context;
	this->render_fences = other.render_fences;
	this->render_semaphores = other.render_semaphores;
	this->present_semaphores = other.present_semaphores;
	this->swapchain_invalid = other.swapchain_invalid;
	this->swapchain = other.swapchain;
	this->swapchain_images = other.swapchain_images;
	this->swapchain_image_views = other.swapchain_image_views;
	this->cmd_buffers = other.cmd_buffers;
	this->current_frame = other.current_frame;

	other.vk_context = {};

	return *this;
}

Renderer::~Renderer()
{
	if (!vk_context)
	{
		return;
	}

	auto const device = vk_context->get_device();

	destroy_swapchain_resources();
	destroy_sync_objects();
	device.destroySwapchainKHR(swapchain);
}

void Renderer::on_swapchain_invalidated()
{
	auto const device = vk_context->get_device();

	destroy_swapchain_resources();

	create_swapchain();

	swapchain_invalid = false;
	device.waitIdle();
}

void Renderer::destroy_swapchain_resources()
{
	auto const device = vk_context->get_device();

	if (!swapchain)
	{
		return;
	}

	device.waitIdle();

	for (auto const &imageView : swapchain_image_views)
	{
		device.destroyImageView(imageView);
	}

	swapchain_image_views.clear();
	swapchain_images.clear();
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

void Renderer::create_swapchain()
{
	auto const device = vk_context->get_device();
	auto const queues = vk_context->get_queues();
	auto const surface = vk_context->get_surface();

	const u32 min_image = surface.capabilities.minImageCount;
	const u32 max_image = surface.capabilities.maxImageCount;

	u32 image_count;
	if ((max_image == 0ul || max_image >= DESIRED_SWAPCHAIN_IMAGES) &&
	    min_image <= DESIRED_SWAPCHAIN_IMAGES)
	{
		image_count = DESIRED_SWAPCHAIN_IMAGES;
	}
	else if (min_image <= 2ul && max_image >= 2ul)
	{
		image_count = 2ul;
	}
	else if (min_image == 0ul && max_image >= 2ul)
	{
		image_count = 2ul;
	}
	else
	{
		image_count = min_image;
	}

	// Create swapchain
	const bool queues_have_same_index = queues.graphics_index == queues.present_index;

	vk::SwapchainKHR old_swapchain = swapchain;

	vk::SwapchainCreateInfoKHR swapchain_info {
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

	for (u32 i = 0; i < swapchain_images.size(); i++)
	{
		swapchain_image_views.push_back(device.createImageView({
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
		}));
	}


	for (u32 i = 0; i < swapchain_images.size(); i++)
	{
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

void Renderer::render_frame(RenderGraph &render_graph, void *user_pointer)
{
	auto const device = vk_context->get_device();

	// Wait for frame's fence
	auto const render_fence = render_fences[current_frame];

	assert_false(device.waitForFences(render_fence, VK_TRUE, UINT64_MAX));

	device.resetFences(render_fence);

	// has_armor()
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
	render_graph.update_and_render(cmd, current_frame, image_index, user_pointer);

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

void Renderer::present_frame(u32 image_index)
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
} // namespace BINDLESSVK_NAMESPACE
