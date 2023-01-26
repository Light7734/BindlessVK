#include "BindlessVk/Renderer.hpp"

#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

Renderer::Renderer(VkContext* vk_context): vk_context(vk_context)
{
	create_sync_objects();
	create_descriptor_pools();
	create_cmd_buffers();
	on_swapchain_invalidated();
}

Renderer::Renderer(Renderer&& other)
{
	*this = std::move(other);
}

Renderer& Renderer::operator=(Renderer&& rhs)
{
	this->vk_context = rhs.vk_context;
	this->render_graph = rhs.render_graph;
	this->render_fences = rhs.render_fences;
	this->render_semaphores = rhs.render_semaphores;
	this->present_semaphores = rhs.present_semaphores;
	this->is_swapchain_invalid = rhs.is_swapchain_invalid;
	this->swapchain = rhs.swapchain;
	this->swapchain_images = rhs.swapchain_images;
	this->swapchain_image_views = rhs.swapchain_image_views;
	this->descriptor_pool = rhs.descriptor_pool;
	this->cmd_buffers = rhs.cmd_buffers;
	this->current_frame = rhs.current_frame;

	rhs.vk_context = {};
	return *this;
}

Renderer::~Renderer()
{
	const auto device = vk_context->get_device();

	if (!vk_context)
	{
		return;
	}

	destroy_swapchain_resources();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		device.destroyFence(render_fences[i]);
		device.destroySemaphore(render_semaphores[i]);
		device.destroySemaphore(present_semaphores[i]);
	}

	render_graph.reset();
	device.resetDescriptorPool(descriptor_pool);
	device.destroyDescriptorPool(descriptor_pool);

	device.destroySwapchainKHR(swapchain);
}

void Renderer::on_swapchain_invalidated()
{
	const auto device = vk_context->get_device();

	destroy_swapchain_resources();

	create_swapchain();

	render_graph.on_swapchain_invalidated(swapchain_images, swapchain_image_views);

	is_swapchain_invalid = false;
	device.waitIdle();
}

void Renderer::destroy_swapchain_resources()
{
	const auto device = vk_context->get_device();

	if (!swapchain)
	{
		return;
	}

	device.waitIdle();

	for (const auto& imageView : swapchain_image_views)
	{
		device.destroyImageView(imageView);
	}

	swapchain_image_views.clear();
	swapchain_images.clear();
};

void Renderer::create_sync_objects()
{
	const auto device = vk_context->get_device();

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		render_fences[i] = device.createFence({
		    vk::FenceCreateFlagBits::eSignaled,
		});

		render_semaphores[i] = device.createSemaphore({});
		present_semaphores[i] = device.createSemaphore({});
	}
}

void Renderer::create_descriptor_pools()
{
	const auto device = vk_context->get_device();

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

	descriptor_pool = device.createDescriptorPool({
	    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
	        | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
	    100,
	    pool_sizes,
	});
}

void Renderer::create_swapchain()
{
	const auto device = vk_context->get_device();
	const auto queues = vk_context->get_queues();
	const auto surface = vk_context->get_surface();

	const u32 min_image = surface.capabilities.minImageCount;
	const u32 max_image = surface.capabilities.maxImageCount;

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
	const auto device = vk_context->get_device();

	cmd_buffers.resize(BVK_MAX_FRAMES_IN_FLIGHT);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		cmd_buffers[i] = device.allocateCommandBuffers({
		    vk_context->get_cmd_pool(i),
		    vk::CommandBufferLevel::ePrimary,
		    1u,
		})[0];
	}
}

void Renderer::build_render_graph(
    std::string backbuffer_name,
    std::vector<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
    std::vector<Renderpass::CreateInfo> renderpasses,
    void (*on_update)(VkContext*, RenderGraph*, u32, void*),
    void (*on_begin_frame)(VkContext*, RenderGraph*, u32, void*),
    vk::DebugUtilsLabelEXT graph_update_debug_label,
    vk::DebugUtilsLabelEXT graph_backbuffer_barrier_debug_label
)
{
	const auto device = vk_context->get_device();
	render_graph.init(vk_context, descriptor_pool, swapchain_images, swapchain_image_views);
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
	const auto device = vk_context->get_device();

	if (is_swapchain_invalid)
	{
		return;
	}

	// Wait for frame's fence
	const auto render_fence = render_fences[current_frame];

	assert_false(device.waitForFences(render_fence, VK_TRUE, UINT64_MAX));

	device.resetFences(render_fence);

	// Aquire next swapchain image
	const auto render_semaphore = render_semaphores[current_frame];
	const auto [result, image_index] =
	    device.acquireNextImageKHR(swapchain, UINT64_MAX, render_semaphore, VK_NULL_HANDLE);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR
	    || is_swapchain_invalid) [[unlikely]]
	{
		device.waitIdle();
		is_swapchain_invalid = true;
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
	const auto cmd = cmd_buffers[current_frame];
	device.resetCommandPool(vk_context->get_cmd_pool(current_frame));
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
	const auto queues = vk_context->get_queues();

	cmd.end();

	vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
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

void Renderer::present_frame(vk::Semaphore wait_semaphore, u32 image_index)
{
	const auto device = vk_context->get_device();
	const auto queues = vk_context->get_queues();

	try
	{
		vk::Result result = queues.present.presentKHR({
		    wait_semaphore,
		    swapchain,
		    image_index,
		});

		if (result == vk::Result::eSuboptimalKHR || is_swapchain_invalid)
		{
			device.waitIdle();
			is_swapchain_invalid = true;
			return;
		}
	}
	// OutOfDateKHR is not considered a success value and throws an error (presentKHR)
	catch (vk::OutOfDateKHRError err)
	{
		device.waitIdle();
		is_swapchain_invalid = true;
		return;
	}
}
} // namespace BINDLESSVK_NAMESPACE
