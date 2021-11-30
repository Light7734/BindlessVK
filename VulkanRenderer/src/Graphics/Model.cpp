#include "Graphics/Model.h"

#include "Graphics/Image.h"

#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "Graphics/RendererPrograms/ModelRendererProgram.h"

#include <tiny_obj_loader.h>

Model::Model(Device* device, const std::string& modelPath, const std::string& baseColorTexPath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn, err;

	m_Image = std::make_unique<Image>(device, baseColorTexPath);

	ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()), "Failed to load model: {}", modelPath);

	for (const auto shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			ModelRendererProgram::Vertex vertex;
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
			};
			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
			};

			m_Vertices.push_back(vertex);
			m_Indices.push_back(m_Indices.size());
		}
	}
}
