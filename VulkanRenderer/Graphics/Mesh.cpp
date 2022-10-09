#include "Graphics/Mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

MeshSystem::MeshSystem(const MeshSystem::CreateInfo& info)
    : m_LogicalDevice(info.deviceContext.logicalDevice)
    , m_PhysicalDevice(info.deviceContext.physicalDevice)
    , m_Allocator(info.deviceContext.allocator)
    , m_CommandPool(info.commandPool)
    , m_GraphicsQueue(info.graphicsQueue)
{
}

void MeshSystem::LoadMesh(const Mesh::CreateInfo& info)
{
	uint32_t hash;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, info.modelPath.c_str()))
	{
		throw std::runtime_error(err);
	}

	// @todo the worst hashing ever, do something proper
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex;

			vertex.position = { attrib.vertices[3 * index.vertex_index + 0],
				                attrib.vertices[3 * index.vertex_index + 1],
				                attrib.vertices[3 * index.vertex_index + 2] };

			vertex.color = glm::vec4({ 1.0, 1.0, 1.0, 1.0 });

			vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],
				          1.0 - attrib.texcoords[2 * index.texcoord_index + 1] };

			if (uniqueVertices.count(vertex) == 0u)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
			hash ^= uniqueVertices[vertex];
			hash *= vertex.color.x - vertex.color.y / vertex.color.z;
		}
	}

	BufferCreateInfo vertexBufferCreateInfo {
		m_LogicalDevice,
		m_PhysicalDevice,
		m_Allocator,
		m_CommandPool,
		m_GraphicsQueue,
		vk::BufferUsageFlagBits::eVertexBuffer,
		vertices.size() * sizeof(Vertex),
		vertices.data(),
	};

	BufferCreateInfo indexBufferCreateInfo {
		m_LogicalDevice,
		m_PhysicalDevice,
		m_Allocator,
		m_CommandPool,
		m_GraphicsQueue,
		vk::BufferUsageFlagBits::eIndexBuffer,
		indices.size() * sizeof(uint32_t),
		indices.data()
	};

	m_Meshes.emplace(HashStr(info.name),
	                 Mesh { hash, StagingBuffer(vertexBufferCreateInfo), StagingBuffer(indexBufferCreateInfo), vertices, indices, {} });
}

MeshSystem::~MeshSystem()
{
}
