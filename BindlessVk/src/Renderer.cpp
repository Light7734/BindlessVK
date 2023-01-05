#include "BindlessVk/Renderer.hpp"

#include "BindlessVk/Types.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

void Renderer::reset()
{
	if (!m_device)
	{
		return;
	}

	destroy_swapchain_resources();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_device->logical.destroyFence(m_render_fences[i]);
		m_device->logical.destroySemaphore(m_render_semaphores[i]);
		m_device->logical.destroySemaphore(m_present_semaphores[i]);
	}

	m_render_graph.reset();
	m_device->logical.resetDescriptorPool(m_descriptor_pool);
	m_device->logical.destroyDescriptorPool(m_descriptor_pool);

	m_device->logical.destroySwapchainKHR(m_swapchain);
}

void Renderer::init(Device* device)
{
	m_device = device;

	create_sync_objects();
	create_descriptor_pools();
	create_cmd_buffers();
	on_swapchain_invalidated();
}

void Renderer::on_swapchain_invalidated()
{
	destroy_swapchain_resources();

	create_swapchain();

	m_render_graph.on_swapchain_invalidated(m_swapchain_images, m_swapchain_image_views);

	m_is_swapchain_invalid = false;
	m_device->logical.waitIdle();
}

void Renderer::destroy_swapchain_resources()
{
	if (!m_swapchain)
	{
		return;
	}

	m_device->logical.waitIdle();

	for (const auto& imageView : m_swapchain_image_views)
	{
		m_device->logical.destroyImageView(imageView);
	}

	m_swapchain_image_views.clear();
	m_swapchain_images.clear();
};

void Renderer::create_sync_objects()
{
	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_render_fences[i] = m_device->logical.createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});

		m_render_semaphores[i] = m_device->logical.createSemaphore({});
		m_present_semaphores[i] = m_device->logical.createSemaphore({});
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

	m_descriptor_pool = m_device->logical.createDescriptorPool({
	    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
	        | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    100,
	    pool_sizes,
	});
}

void Renderer::create_swapchain()
{
	const u32 min_image = m_device->surface_capabilities.minImageCount;
	const u32 max_image = m_device->surface_capabilities.maxImageCount;

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
	bool same_queue_index = m_device->graphics_queue_index == m_device->present_queue_index;

	vk::SwapchainKHR old_swapchain = m_swapchain;

	vk::SwapchainCreateInfoKHR swapchain_info {
		{},
		m_device->surface,
		image_count,
		m_device->surface_format.format,
		m_device->surface_format.colorSpace,
		m_device->framebuffer_extent,
		1u,
		vk::ImageUsageFlagBits::eColorAttachment,
		same_queue_index ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
		same_queue_index ? 0u : 2u,
		same_queue_index ? nullptr : &m_device->graphics_queue_index,
		m_device->surface_capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		m_device->present_mode,
		VK_TRUE,
		old_swapchain,
	};

	m_swapchain = m_device->logical.createSwapchainKHR(swapchain_info, nullptr);
	m_device->logical.destroySwapchainKHR(old_swapchain);

	m_swapchain_images = m_device->logical.getSwapchainImagesKHR(m_swapchain);

	for (u32 i = 0; i < m_swapchain_images.size(); i++)
	{
		m_swapchain_image_views.push_back(m_device->logical.createImageView({
		    {},
		    m_swapchain_images[i],
		    vk::ImageViewType::e2D,
		    m_device->surface_format.format,
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


	for (u32 i = 0; i < m_swapchain_images.size(); i++)
	{
		m_device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImage,
		    (u64)(VkImage)m_swapchain_images[i],
		    fmt::format("swap_chain_image_{}", i).c_str(),
		});

		m_device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    vk::ObjectType::eImageView,
		    (u64)(VkImageView)m_swapchain_image_views[i],
		    fmt::format("swapchain_image_view_{}", i).c_str(),
		});
	}
}

void Renderer::create_cmd_buffers()
{
	m_command_buffers.resize(BVK_MAX_FRAMES_IN_FLIGHT);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_command_buffers[i] = m_device->logical.allocateCommandBuffers({
		    m_device->get_cmd_pool(i),
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
	m_render_graph.init(m_device, m_descriptor_pool, m_swapchain_images, m_swapchain_image_views);
	m_render_graph.build(
	    backbuffer_name,
	    buffer_inputs,
	    renderpasses,
	    on_update,
	    on_begin_frame,
	    graph_update_debug_label,
	    graph_backbuffer_barrier_debug_label
	);
}

void Renderer::begin_frame(void* userPointer)
{
	m_render_graph.begin_frame(m_current_frame, userPointer);
}

void Renderer::end_frame(void* userPointer)
{
	if (m_is_swapchain_invalid)
	{
		return;
	}

	// Wait for frame's fence
	auto render_fence = m_render_fences[m_current_frame];
	BVK_ASSERT(m_device->logical.waitForFences(render_fence, VK_TRUE, UINT64_MAX));
	m_device->logical.resetFences(render_fence);


	// Aquire next swapchain image
	auto render_semaphore = m_render_semaphores[m_current_frame];
	auto [result, image_index] =
	    m_device->logical
	        .acquireNextImageKHR(m_swapchain, UINT64_MAX, render_semaphore, VK_NULL_HANDLE);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR
	    || m_is_swapchain_invalid) [[unlikely]]
	{
		m_device->logical.waitIdle();
		m_is_swapchain_invalid = true;
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
	auto cmd = m_command_buffers[m_current_frame];
	m_device->logical.resetCommandPool(m_device->get_cmd_pool(m_current_frame));
	m_render_graph.end_frame(cmd, m_current_frame, image_index, userPointer);

	// Submit & present
	auto present_semaphore = m_present_semaphores[m_current_frame];
	submit_queue(render_semaphore, present_semaphore, render_fence, cmd);
	present_frame(m_present_semaphores[m_current_frame], image_index);

	// ++
	m_current_frame = (m_current_frame + 1u) % BVK_MAX_FRAMES_IN_FLIGHT;
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
	m_device->graphics_queue.submit(
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
		vk::Result result = m_device->present_queue.presentKHR({
		    wait_semaphore,
		    m_swapchain,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || m_is_swapchain_invalid)
		{
			m_device->logical.waitIdle();
			m_is_swapchain_invalid = true;
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		m_device->logical.waitIdle();
		m_is_swapchain_invalid = true;
		return;
	}
}
} // namespace BINDLESSVK_NAMESPACE
