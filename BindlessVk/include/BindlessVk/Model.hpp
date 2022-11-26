#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Texture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace BINDLESSVK_NAMESPACE {

namespace VertexTypes {

struct Model
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 uv;
	glm::vec3 color;

	static constexpr std::array<vk::VertexInputBindingDescription, 1> GetBindings()
	{
		return {
			vk::VertexInputBindingDescription {
			    0u,                                                // binding
			    static_cast<uint32_t>(sizeof(VertexTypes::Model)), // stride
			    vk::VertexInputRate::eVertex,                      // inputRate
			},
		};
	}

	static constexpr std::array<vk::VertexInputAttributeDescription, 5> GetAttributes()
	{
		return {
			vk::VertexInputAttributeDescription {
			    0u,                                     // location
			    0u,                                     // binding
			    vk::Format::eR32G32B32Sfloat,           // format
			    offsetof(VertexTypes::Model, position), // offset
			},
			vk::VertexInputAttributeDescription {
			    1u,                                   // location
			    0u,                                   // binding
			    vk::Format::eR32G32B32Sfloat,         // format
			    offsetof(VertexTypes::Model, normal), // offset
			},
			vk::VertexInputAttributeDescription {
			    2u,                                    // location
			    0u,                                    // binding
			    vk::Format::eR32G32B32Sfloat,          // format
			    offsetof(VertexTypes::Model, tangent), // offset
			},
			vk::VertexInputAttributeDescription {
			    3u,                               // location
			    0u,                               // binding
			    vk::Format::eR32G32Sfloat,        // format
			    offsetof(VertexTypes::Model, uv), // offset
			},
			vk::VertexInputAttributeDescription {
			    4u,                                  // location
			    0u,                                  // binding
			    vk::Format::eR32G32B32Sfloat,        // format
			    offsetof(VertexTypes::Model, color), // offset
			},


		};
		offsetof(VertexTypes::Model, uv);
	}

	static vk::PipelineVertexInputStateCreateInfo GetVertexInputState()
	{
		static const auto bindings   = GetBindings();
		static const auto attributes = GetAttributes();

		return vk::PipelineVertexInputStateCreateInfo {
			{},
			static_cast<uint32_t>(bindings.size()),
			bindings.data(),

			static_cast<uint32_t>(attributes.size()),
			attributes.data(),
		};
	}
};

} // namespace VertexTypes

struct Model
{
public:
	struct CreateInfo
	{
		TextureSystem& textureSystem;
		const char* name;
		const char* gltfPath;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
		glm::vec3 color; // colour matey
	};

	struct Primitive
	{
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	struct Node
	{
		~Node()
		{
			for (auto* node : children)
				delete node;
		}

		Node* parent;
		std::vector<Node*> children;

		std::vector<Primitive> mesh;
		glm::mat4 transform;
	};

	struct MaterialParameters
	{
		glm::vec3 albedoFactor   = glm::vec4(1.0f);
		glm::vec3 diffuseFactor  = glm::vec4(1.0f);
		glm::vec3 specularFactor = glm::vec4(1.0f);

		int32_t albedoTextureIndex;
		int32_t normalTextureIndex;
		int32_t metallicRoughnessTextureIndex;
	};

	struct Texture
	{
		int32_t imageIndex;
	};

	/// @todo: Support textures with different samplers
	std::vector<BINDLESSVK_NAMESPACE::Texture*> textures;
	std::vector<MaterialParameters> materialParameters;
	std::vector<Node*> nodes;
	class StagingBuffer* vertexBuffer;
	class StagingBuffer* indexBuffer;
};

class ModelSystem
{
public:
	struct CreateInfo
	{
		Device* device;
	};

public:
	ModelSystem() = default;
	void Init(const ModelSystem::CreateInfo& info);

	void Reset();

	void LoadModel(const Model::CreateInfo& info);

	inline Model* GetModel(const char* name)
	{
		return &m_Models[HashStr(name)];
	}

private:
	Device* m_Device;
	vk::CommandPool m_CommandPool;
	std::unordered_map<uint64_t, Model> m_Models;
};

} // namespace BINDLESSVK_NAMESPACE
