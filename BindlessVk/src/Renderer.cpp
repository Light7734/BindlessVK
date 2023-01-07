#include "BindlessVk/Renderer.hpp"

#include "BindlessVk/Types.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

void Renderer::reset()
{
	if (!device)
	{
		return;
	}

	destroy_swapchain_resources();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		device->logical.destroyFence(render_fences[i]);
		device->logical.destroySemaphore(render_semaphores[i]);
		device->logical.destroySemaphore(present_semaphores[i]);
	}

	render_graph.reset();
	device->logical.resetDescriptorPool(descriptor_pool);
	device->logical.destroyDescriptorPool(descriptor_pool);

	device->logical.destroySwapchainKHR(swapchain);
}

void Renderer::init(Device* device)
{
	this->device = device;

	create_sync_objects();
	create_descriptor_pools();
	create_cmd_buffers();
	on_swapchain_invalidated();
}

void Renderer::on_swapchain_invalidated()
{
	destroy_swapchain_resources();

	create_swapchain();

	render_graph.on_swapchain_invalidated(swapchain_images, swapchain_image_views);

	is_swapchain_invalid = false;
	device->logical.waitIdle();
}

void Renderer::destroy_swapchain_resources()
{
	if (!swapchain)
	{
		return;
	}

	device->logical.waitIdle();

	for (const auto& imageView : swapchain_image_views)
	{
		device->logical.destroyImageView(imageView);
	}

	swapchain_image_views.clear();
	swapchain_images.clear();
};

void Renderer::create_sync_objects()
{
	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		render_fences[i] = device->logical.createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});

		render_semaphores[i] = device->logical.createSemaphore({});
		present_semaphores[i] = device->logical.createSemaphore({});
	}
}

void Renderer::create_descriptor_pools()
{
	std::vector<vk::DescriptorPoolSize> pool_sizes = {
		{ vk::DescriptorType::eSampler, 8000 },
		{ vk::DescriptorType::eCombinedImageSampler, 8000 },
		{ vk::DescriptorType::eSampledImage, 8000 },
		{ vk::DescriptorType::eStorageImage, 8000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 8000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 8000 },
		{ vk::DescriptorType::eUniformBuffer, 8000 },
		{ vk::DescriptorType::eStorageBuffer, 8000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 8000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 8000 },
		{ vk::DescriptorType::eInputAttachment, 8000 }
	};

	descriptor_pool = device->logical.createDescriptorPool({
	    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
	        | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    100,
	    pool_sizes,
	});
}

void Renderer::create_swapchain()
{
	const u32 min_image = device->surface_capabilities.minImageCount;
	const u32 max_image = device->surface_capabilities.maxImageCount;

	u32 image_count;

	if ((max_image == 0ul || max_image >= DESIRED_SWAPCHAIN_IMAGES)
	    && min_image <= DESIRED_SWAPCHAIN_IMAGES)
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
	bool same_queue_index = device->graphics_queue_index == device->present_queue_index;

	vk::SwapchainKHR old_swapchain = swapchain;

	vk::SwapchainCreateInfoKHR swapchain_info {
		{},
		device->surface,
		image_count,
		device->surface_format.format,
		device->surface_format.colorSpace,
		device->framebuffer_extent,
		1u,
		vk::ImageUsageFlagBits::eColorAttachment,
		same_queue_index ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
		same_queue_index ? 0u : 2u,
		same_queue_index ? nullptr : &device->graphics_queue_index,
		device->surface_capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		device->present_mode,
		VK_TRUE,
		old_swapchain,
	};

	swapchain = device->logical.createSwapchainKHR(swapchain_info, nullptr);
	device->logical.destroySwapchainKHR(old_swapchain);

	swapchain_images = device->logical.getSwapchainImagesKHR(swapchain);

	for (u32 i = 0; i < swapchain_images.size(); i++)
	{
		swapchain_image_views.push_back(device->logical.createImageView({
		    {},
		    swapchain_images[i],
		    vk::ImageViewType::e2D,
		    device->surface_format.format,
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
		device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (u64)(VkImage)swapchain_images[i],
		    fmt::format("swap_chain_image_{}", i).c_str(),
		});

		device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (u64)(VkImageView)swapchain_image_views[i],
		    fmt::format("swapchain_image_view_{}", i).c_str(),
		});
	}
}

void Renderer::create_cmd_buffers()
{
	cmd_buffers.resize(BVK_MAX_FRAMES_IN_FLIGHT);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		cmd_buffers[i] = device->logical.allocateCommandBuffers({
		    device->get_cmd_pool(i),
		    vk::CommandBufferLevel::ePrimary,
		    1u,
		})[0];
	}
}

void Renderer::build_render_graph(
    std::string backbuffer_name,
    std::vector<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
    std::vector<Renderpass::CreateInfo> renderpasses,
    void (*on_update)(Device*, RenderGraph*, u32, void*),
    void (*on_begin_frame)(Device*, RenderGraph*, u32, void*),
    vk::DebugUtilsLabelEXT graph_update_debug_label,
    vk::DebugUtilsLabelEXT graph_backbuffer_barrier_debug_label
)
{
	render_graph.init(device, descriptor_pool, swapchain_images, swapchain_image_views);
	render_graph.build(
	    backbuffer_name,
	    buffer_inputs,
	    renderpasses,
	    on_update,
	    on_begin_frame,
	    graph_update_debug_label,
	    graph_backbuffer_barrier_debug_label
	);
}

void Renderer::begin_frame(void* user_pointer)
{
	render_graph.begin_frame(current_frame, user_pointer);
}

void Renderer::end_frame(void* user_pointer)
{
	if (is_swapchain_invalid)
	{
		return;
	}

	// Wait for frame's fence
	auto render_fence = render_fences[current_frame];


	BVK_ASSERT(device->logical.waitForFences(render_fence, VK_TRUE, UINT64_MAX));

	device->logical.resetFences(render_fence);

	// Aquire next swapchain image
	auto render_semaphore = render_semaphores[current_frame];
	auto [result, image_index] =
	    device->logical
	        .acquireNextImageKHR(swapchain, UINT64_MAX, render_semaphore, VK_NULL_HANDLE);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR
	    || is_swapchain_invalid) [[unlikely]]
	{
		device->logical.waitIdle();
		is_swapchain_invalid = true;
		return;
	}
	else
	{
		BVK_ASSERT(
		    result != vk::Result::eSuccess,
		    "VkAcquireNextImage failed without returning VK_ERROR_OUT_OF_DATE_KHR or "
		    "VK_SUBOPTIMAL_KHR"
		);
	}

	// Draw scene
	auto cmd = cmd_buffers[current_frame];
	device->logical.resetCommandPool(device->get_cmd_pool(current_frame));
	render_graph.end_frame(cmd, current_frame, image_index, user_pointer);

	// Submit & present
	auto present_semaphore = present_semaphores[current_frame];
	submit_queue(render_semaphore, present_semaphore, render_fence, cmd);
	present_frame(present_semaphores[current_frame], image_index);

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
	cmd.end();

	vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	device->graphics_queue.submit(
	    vk::SubmitInfo {
	        wait_semaphore,
	        wait_stage,
	        cmd,
	        signal_semaphore,
	    },
	    signal_fence
	);
}

void Renderer::present_frame(vk::Semaphore wait_semaphore, u32 image_index)
{
	try
	{
		vk::Result result = device->present_queue.presentKHR({
		    wait_semaphore,
		    swapchain,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || is_swapchain_invalid)
		{
			device->logical.waitIdle();
			is_swapchain_invalid = true;
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device->logical.waitIdle();
		is_swapchain_invalid = true;
		return;
	}
}
} // namespace BINDLESSVK_NAMESPACE
