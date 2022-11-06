#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Scene/Camera.hpp"
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
		uint32_t swapchainImageCount;
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

	struct UpdateData
	{
		RenderGraph& graph;
		uint32_t frameIndex;
		vk::Device logicalDevice;
		Scene* scene;
	};

public:
	RenderGraph();

	~RenderGraph();

	void Init(const CreateInfo& info);

	void Build();

	void Update(Scene* scene, uint32_t frameIndex);

	void Render(const RenderPass::RenderData& data);

	/////////////////////////////////////////////////////////////////////////////////
	/// Builder functions
	inline RenderGraph& SetName(const std::string& name)
	{
		m_Name = name;
		return *this;
	}

	inline RenderGraph& AddRenderPassRecipe(const RenderPassRecipe& recipe)
	{
		m_Recipes.push_back(recipe);
		return *this;
	}

	inline RenderGraph& SetSwapchainContext()
	{
	}

	inline RenderGraph& AddBufferInput(const RenderPassRecipe::BufferInputInfo& info)
	{
		ASSERT(info.type == vk::DescriptorType::eUniformBuffer || info.type == vk::DescriptorType::eStorageBuffer,
		       "Invalid descriptor type for buffer input: {}",
		       string_VkDescriptorType(static_cast<VkDescriptorType>(info.type)));
		LOG(trace, "Adding buffer input info: {}", info.name);
		m_BufferInputInfos.push_back(info);
		return *this;
	}

	inline RenderGraph& AddTextureInput()
	{
		ASSERT(false, "Not implemented");
		return *this;
	}

	inline RenderGraph& SetUpdateAction(std::function<void(const RenderGraph::UpdateData&)> action)
	{
		m_UpdateAction = action;
		return *this;
	}

	inline RenderGraph& SetBackbuffer(const std::string& name)
	{
		m_BackbufferAttachmentNames.push_back(name);
		return *this;
	}

	inline void* MapDescriptorBuffer(const char* name, uint32_t frameIndex)
	{
		uint8_t* map = (uint8_t*)(m_BufferInputs[HashStr(name)]->Map());

		// @todo: stop this hacc
		LOG(warn, "...");
		for (uint32_t i = 0; i < m_BufferInputInfos.size(); i++)
		{
			LOG(warn, "{} =? {}", m_BufferInputInfos[i].name, name);
			if (m_BufferInputInfos[i].name == name)
			{
				LOG(warn, "{} -> {} + {} * {}", name, (size_t)map, m_BufferInputInfos[i].size, frameIndex);
				map += m_BufferInputInfos[i].size * frameIndex;
			}
		}

		return (void*)map;
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
		enum class Type
		{
			ePerImage,
			ePerFrame,
			eSingle,
		} type;

		// Required for aliasing Read-Modify-Write attachments
		std::string lastWriteName;

		vk::AccessFlags accessMask;
		vk::PipelineStageFlags stageMask;
		vk::ImageLayout layout;
		vk::Format format;

		VkExtent3D extent;
		glm::vec2 size;
		RenderPassRecipe::SizeType sizeType;
		std::string relativeSizeName;

		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;

		// Transient multi-sampled images
		vk::SampleCountFlagBits sampleCount;
		vk::ResolveModeFlagBits transientMSResolveMode;
		AllocatedImage transientMSImage;
		vk::ImageView transientMSImageView;

		std::tuple<vk::Image, vk::ImageView, vk::Image, vk::ImageView> GetResource(uint32_t imageIndex, uint32_t frameIndex)
		{
			switch (type)
			{
			case Type::ePerImage:
			{
				return std::make_tuple(
				    images[imageIndex],
				    imageViews[imageIndex],
				    transientMSImage,
				    transientMSImageView);
			}

			case Type::ePerFrame:
			{
				return std::make_tuple(
				    images[frameIndex],
				    imageViews[frameIndex],
				    transientMSImage,
				    transientMSImageView);
			}

			case Type::eSingle:
			{
				return std::make_tuple(
				    images[0],
				    imageViews[0],
				    transientMSImage,
				    transientMSImageView);
			}

			default:
				ASSERT(false, "Invalid attachment resource type");
				return {};
			}
		};
	};

	void CreateAttachmentResource(const RenderPassRecipe::AttachmentInfo& info, RenderGraph::AttachmentResource::Type type);


	std::string m_Name = {};

	std::function<void(const RenderGraph::UpdateData&)> m_UpdateAction = {};

	std::vector<AttachmentResource> m_AttachmentResources = {};

	std::vector<RenderPassRecipe> m_Recipes = {};
	std::vector<RenderPass> m_RenderPasses  = {};
	vk::Extent2D m_SwapchainExtent          = {};

	std::string m_BackbufferResourceName = {};
	std::vector<std::string> m_BackbufferAttachmentNames;

	std::vector<vk::Image> m_SwapchainImages         = {};
	std::vector<vk::ImageView> m_SwapchainImageViews = {};

	vk::Format m_ColorFormat;
	vk::Format m_DepthFormat;

	std::vector<RenderPassRecipe::BufferInputInfo> m_BufferInputInfos;

	uint32_t m_MinUniformBufferOffsetAlignment;

	////
	std::unordered_map<uint64_t, Buffer*> m_BufferInputs = {};

	vma::Allocator m_Allocator = {};

	std::vector<vk::DescriptorSet> m_DescriptorSets = {};

	// std::vector<RenderPass::DescriptorInfo> m_DescriptorInfos = {};

	uint32_t m_SwapchainImageCount      = {};
	vk::DescriptorPool m_DescriptorPool = {};

	vk::PipelineLayout m_PipelineLayout           = {};
	vk::DescriptorSetLayout m_DescriptorSetLayout = {};

	vk::Device m_LogicalDevice          = {};
	vk::PhysicalDevice m_PhysicalDevice = {};
	vk::CommandPool m_CommandPool       = {};
	QueueInfo m_QueueInfo               = {};
};
