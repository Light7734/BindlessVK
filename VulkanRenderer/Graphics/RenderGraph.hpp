#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Scene/CameraController.hpp"
// #include "Graphics/Pipeline.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif


class RenderGraph
{
public:
	struct CreateInfo
	{
		vk::Extent2D swapchainExtent;
		vk::DescriptorPool descriptorPool;
		vk::Device logicalDevice;
		vk::PhysicalDevice physicalDevice;
		vma::Allocator allocator;
		vk::CommandPool commandPool;
		vk::Format colorFormat;
		vk::Format depthFormat;
		QueueInfo queueInfo;
		std::vector<vk::Image> swapchainImages;
		std::vector<vk::ImageView> swapchainImageViews;
	};

	struct BuildInfo
	{
		std::string backbufferName;

		std::vector<RenderPass::CreateInfo::BufferInputInfo> bufferInputs;

		std::vector<RenderPass::CreateInfo> renderPasses;

		std::function<void(const RenderContext&)> updateAction;
	};

public:
	RenderGraph();

	~RenderGraph();

	void Init(const CreateInfo& info);

	void Build(const BuildInfo& info);

	void Render(RenderContext context);

	inline void* MapDescriptorBuffer(const char* name, uint32_t frameIndex)
	{
		return m_BufferInputs[HashStr(name)]->MapBlock(frameIndex);
	}

	inline void UnMapDescriptorBuffer(const char* name)
	{
		m_BufferInputs[HashStr(name)]->Unmap();
	}

private:
	void ValidateGraph();
	void ReorderPasses();

	void BuildAttachmentResources();

	void BuildTextureInputs();
	void BuildBufferInputs();

	void BuildDescriptorSets();
	void WriteDescriptorSets();

private:
	struct ImageAttachment
	{
		std::vector<AllocatedImage> image;
		std::vector<vk::ImageView> imageView;

		// Transinet multi-sampled
		std::vector<AllocatedImage> transientMSImage;
		std::vector<vk::ImageView> transientMSImageView;
	};

	struct AttachmentResource
	{
		// State carried from last barrier
		vk::AccessFlags srcAccessMask;
		vk::ImageLayout srcImageLayout;
		vk::PipelineStageFlags srcStageMask;

		vk::Image image;
		vk::ImageView imageView;
	};

	struct AttachmentResourceContainer
	{
		enum class Type
		{
			ePerImage,
			ePerFrame,
			eSingle,
		} type;

		vk::Format imageFormat;

		VkExtent3D extent;
		glm::vec2 size;
		RenderPass::CreateInfo::SizeType sizeType;
		std::string relativeSizeName;

		// Transient multi-sampled images
		vk::SampleCountFlagBits sampleCount;
		vk::ResolveModeFlagBits transientMSResolveMode;
		AllocatedImage transientMSImage;
		vk::ImageView transientMSImageView;

		// Required for aliasing Read-Modify-Write attachments
		std::string lastWriteName;

		std::vector<AttachmentResource> resources;

		inline AttachmentResource& GetResource(uint32_t imageIndex, uint32_t frameIndex)
		{
			return type == Type::ePerImage ? resources[imageIndex] :
			       type == Type::ePerFrame ? resources[frameIndex] :
			                                 resources[0];
		};
	};

	void CreateAttachmentResource(const RenderPass::CreateInfo::AttachmentInfo& info, RenderGraph::AttachmentResourceContainer::Type type);

private:
	vk::Device m_LogicalDevice          = {};
	vk::PhysicalDevice m_PhysicalDevice = {};
	vma::Allocator m_Allocator          = {};

	vk::CommandPool m_CommandPool       = {};
	vk::DescriptorPool m_DescriptorPool = {};

	vk::PhysicalDeviceProperties m_PhysicalDeviceProperties = {};

	QueueInfo m_QueueInfo = {};


	std::vector<RenderPass> m_RenderPasses = {};

	vk::PipelineLayout m_PipelineLayout             = {};
	vk::DescriptorSetLayout m_DescriptorSetLayout   = {};
	std::vector<vk::DescriptorSet> m_DescriptorSets = {};

	std::function<void(const RenderContext&)> m_UpdateAction = {};

	std::vector<AttachmentResourceContainer> m_AttachmentResources = {};

	std::unordered_map<uint64_t, Buffer*> m_BufferInputs = {};

	std::vector<vk::Image> m_SwapchainImages         = {};
	std::vector<vk::ImageView> m_SwapchainImageViews = {};
	vk::Extent2D m_SwapchainExtent                   = {};

	std::vector<std::string> m_SwapchainAttachmentNames = {};
	std::string m_SwapchainResourceName                 = {};
	uint32_t m_SwapchainResourceIndex                   = {};

	vk::Format m_ColorFormat = {};
	vk::Format m_DepthFormat = {};

	std::vector<RenderPass::CreateInfo> m_RenderPassCreateInfos = {};
	std::vector<RenderPass::CreateInfo::BufferInputInfo> m_BufferInputInfos;
};
