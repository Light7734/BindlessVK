#include "Graphics/Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(ModelCreateInfo& createInfo)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, createInfo.modelPath.c_str()))
	{
		throw std::runtime_error(err);
	}

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

			if (m_UniqueVertices.count(vertex) == 0u)
			{
				m_UniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
				m_Vertices.push_back(vertex);
			}

			m_Indices.push_back(m_UniqueVertices[vertex]);
		}
	}
}

Model::~Model()
{
}
