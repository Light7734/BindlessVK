#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Scene/Camera.hpp"
// #include "Graphics/Pipeline.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"

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

class RenderPass
{
public:
	struct AttachmentInfo
	{
		std::string name;

		std::vector<vk::Image> images;
		std::vector<vk::ImageView> views;
		vk::ImageLayout layout;

		vk::ResolveModeFlagBits resolveMode    = vk::ResolveModeFlagBits::eNone;
		vk::ImageLayout resolveLayout          = vk::ImageLayout::eUndefined;
		std::vector<vk::ImageView> resolveView = {};

		vk::AttachmentLoadOp loadOp   = vk::AttachmentLoadOp::eLoad;
		vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;


		vk::PipelineStageFlags stageMask;
		vk::AccessFlags accessMask;

		vk::ImageSubresourceRange subresourceRange = {
			vk::ImageAspectFlagBits::eColor, // aspectMask
			0u,                              // baseMipLevel
			1u,                              // levelCount
			0u,                              //baseArrayLayer
			1u,                              // layerCount
		};
	};

	struct DescriptorInfo
	{
		std::string name;
		uint32_t count;
		vk::DescriptorType type;
		vk::ShaderStageFlags stageMask;

		size_t bufferSize = 0ull;
		void* initialData = nullptr;
	};


	struct RenderData
	{
		class Scene* scene;
		vk::CommandBuffer cmd;
		uint32_t imageIndex;
		uint32_t frameIndex;
	};

	struct UpdateData
	{
		RenderPass& renderPass;
		uint32_t frameIndex;
		vk::Device logicalDevice;
		class Scene* scene;
	};

public:
	RenderPass()
	{
	}

	~RenderPass()
	{
	}

	inline void Render(const RenderData& data) { m_RenderAction(data); }
	inline void Update(const UpdateData& data) { m_UpdateAction(data); }

	inline RenderPass& SetName(const std::string& name)
	{
		if (name.empty())
		{
			LOG(warn, "Render pass set name to empty");
		}

		m_Name = name;
		return *this;
	}

	const std::string& GetName() { return m_Name; }

	inline RenderPass& SetRenderAction(std::function<void(const RenderData&)>&& renderAction)
	{
		m_RenderAction = renderAction;
		return *this;
	}

	inline RenderPass& SetUpdateAction(std::function<void(const UpdateData&)>&& updateAction)
	{
		m_UpdateAction = updateAction;
		return *this;
	}

	inline const std::vector<RenderPass::AttachmentInfo>& GetAttachments() const
	{
		return m_ColorAttachments;
	}

	inline const std::vector<RenderPass::DescriptorInfo>& GetDescriptorInfos() const
	{
		return m_Descriptors;
	}

	inline RenderPass& AddDescriptor(const RenderPass::DescriptorInfo& info)
	{
		m_Descriptors.push_back(info);
		return *this;
	}

	inline RenderPass& AddColorAttachment(const RenderPass::AttachmentInfo& info)
	{
		m_ColorAttachments.push_back(info);
		return *this;
	}

	inline RenderPass& SetDepthAttachment(const RenderPass::AttachmentInfo& info)
	{
		m_DepthAttachment = info;
		return *this;
	}

	inline void SetPipelineLayout(vk::PipelineLayout pipelineLayout)
	{
		m_PipelineLayout = pipelineLayout;
	}

	inline void SetDescriptorSet(vk::DescriptorSet set, uint32_t frameIndex)
	{
		m_DescriptorSets[frameIndex] = set;
	}

	inline void SetDescriptorResource(const char* name, uint32_t frameIndex, Buffer* resource)
	{
		m_DescriptorResources[HashStr(name)][frameIndex] = resource;
	};

	vk::DescriptorSet GetDescriptorSet(uint32_t frameIndex) const { return m_DescriptorSets[frameIndex]; };
	vk::PipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
	std::string m_Name = {};
	std::function<void(const RenderData&)> m_RenderAction;
	std::function<void(const UpdateData&)> m_UpdateAction;

	std::vector<RenderPass::DescriptorInfo> m_Descriptors;
	std::unordered_map<uint64_t, std::array<Buffer*, MAX_FRAMES_IN_FLIGHT>> m_DescriptorResources;

	std::vector<RenderPass::AttachmentInfo> m_ColorAttachments;
	RenderPass::AttachmentInfo m_DepthAttachment;

	std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> m_DescriptorSets;
	vk::DescriptorSetLayout m_DescriptorSetLayout;
	vk::PipelineLayout m_PipelineLayout;
};
