#pragma once

#include "Core/Base.hpp"
#include "Core/UUID.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Texture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <unordered_map>

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
		glm::vec4 baseColor = glm::vec4(1.0f);
		int32_t baseColorTextureIndex;
	};

	struct Image
	{
		::Texture* texture;
	};

	struct Texture
	{
		int32_t imageIndex;
	};

	std::vector<Image> images;
	std::vector<Model::Texture> textures;
	std::vector<MaterialParameters> materialParameters;
	std::vector<Node*> nodes;
	StagingBuffer* vertexBuffer;
	StagingBuffer* indexBuffer;
};

class ModelSystem
{
public:
	struct CreateInfo
	{
		DeviceContext deviceContext;
		vk::CommandPool commandPool;
		vk::Queue graphicsQueue;
	};

public:
	ModelSystem(const ModelSystem::CreateInfo& info);
	ModelSystem() = default;

	~ModelSystem();

	void LoadModel(const Model::CreateInfo& info);

	inline Model* GetModel(const char* name) { return &m_Models[HashStr(name)]; }

private:
	vk::Device m_LogicalDevice;
	vk::PhysicalDevice m_PhysicalDevice;
	vma::Allocator m_Allocator;
	vk::CommandPool m_CommandPool;
	vk::Queue m_GraphicsQueue;


	std::unordered_map<uint64_t, Model> m_Models;
};
