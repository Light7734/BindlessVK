#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common.hpp"
#include "BindlessVk/Types.hpp"

#include <glm/glm.hpp>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

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

			class Texture* defaultTexture;
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

		std::function<void(const RenderContext& context)> onUpdate;
		std::function<void(const RenderContext& context)> onRender;
		std::function<void(const RenderContext& context)> onBeginFrame;

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

	std::unordered_map<uint64_t, class Buffer*> bufferInputs;

	std::vector<vk::DescriptorSet> descriptorSets;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;

	std::function<void(const RenderContext& context)> onRender;
	std::function<void(const RenderContext& context)> onUpdate;
	std::function<void(const RenderContext& context)> onBeginFrame;

	void* userPointer;
};

} // namespace BINDLESSVK_NAMESPACE
