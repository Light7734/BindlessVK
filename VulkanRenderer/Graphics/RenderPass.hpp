#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"
#include "Scene/CameraController.hpp"

#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

struct RenderPass
{
	struct CreateInfo
	{
	public:
		enum class SizeType : uint8_t
		{
			eSwapchainRelative,
			eRelative,
			eAbsolute,

			nCount,
		};

		struct AttachmentInfo
		{
			std::string name;
			glm::vec2 size;
			SizeType sizeType;
			vk::Format format;
			vk::SampleCountFlagBits samples;

			std::string input;
			std::string sizeRelativeName;

			vk::ClearValue clearValue;
		};

		struct TextureInputInfo
		{
			std::string name;
			uint32_t binding;
			uint32_t count;
			vk::DescriptorType type;
			vk::ShaderStageFlagBits stageMask;

			Texture* defaultTexture;
		};

		struct BufferInputInfo
		{
			std::string name;
			uint32_t binding;
			uint32_t count;
			vk::DescriptorType type;
			vk::ShaderStageFlagBits stageMask;

			size_t size;
			void* initialData;
		};

		std::string name;

		std::function<void(const RenderContext& context)> renderAction;
		std::function<void(const RenderContext& context)> updateAction;

		std::vector<CreateInfo::AttachmentInfo> colorAttachmentInfos;
		CreateInfo::AttachmentInfo depthStencilAttachmentInfo;

		std::vector<CreateInfo::TextureInputInfo> textureInputInfos;
		std::vector<CreateInfo::BufferInputInfo> bufferInputInfos;

		void* userPointer;
	};

	struct Attachment
	{
		vk::PipelineStageFlags stageMask;
		vk::AccessFlags accessMask;
		vk::ImageLayout layout;
		vk::ImageSubresourceRange subresourceRange;

		vk::AttachmentLoadOp loadOp;
		vk::AttachmentStoreOp storeOp;

		uint32_t resourceIndex;

		vk::ClearValue clearValue;
	};

	std::string name;

	std::vector<Attachment> attachments;

	std::unordered_map<uint64_t, Buffer*> bufferInputs;

	std::vector<vk::DescriptorSet> descriptorSets;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;

	std::function<void(const RenderContext& context)> renderAction;
	std::function<void(const RenderContext& context)> updateAction;

	void* userPointer;
};
