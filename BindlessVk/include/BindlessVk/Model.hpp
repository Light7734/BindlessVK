#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Texture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace BINDLESSVK_NAMESPACE {

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
		vk::CommandPool commandPool;
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
