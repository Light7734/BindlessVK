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

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 uv;
	uint32_t objectIndex;

	inline bool operator==(const Vertex& other) const
	{
		return position == other.position &&
		       color == other.color &&
		       uv == other.uv;
	}
};

struct ObjectData
{
	glm::mat4 transform;
};

namespace std {
template<>
struct hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
		       (hash<glm::vec2>()(vertex.uv) << 1);
	}
};

} // namespace std

struct Mesh
{
	struct CreateInfo
	{
		const char* name;
		std::string modelPath;
		std::vector<std::shared_ptr<Texture>> textures;
	};

	uint32_t hash;

	StagingBuffer vertexBuffer;
	StagingBuffer indexBufer;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<Texture*> textures;
};


class MeshSystem
{
public:
	struct CreateInfo
	{
		DeviceContext deviceContext;
		vk::CommandPool commandPool;
		vk::Queue graphicsQueue;
	};

public:
	MeshSystem(const MeshSystem::CreateInfo& info);
	MeshSystem() = default;

	~MeshSystem();

	void LoadMesh(const Mesh::CreateInfo& info);

	inline Mesh* GetMesh(const char* name) { return &m_Meshes[HashStr(name)]; }

private:
	vk::Device m_LogicalDevice;
	vk::PhysicalDevice m_PhysicalDevice;
	vma::Allocator m_Allocator;
	vk::CommandPool m_CommandPool;
	vk::Queue m_GraphicsQueue;


	std::unordered_map<uint64_t, Mesh> m_Meshes;
};
