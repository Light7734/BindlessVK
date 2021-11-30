#pragma once

#include "Core/Base.h"
#include "Graphics/Image.h"
#include "Graphics/RendererPrograms/ModelRendererProgram.h"

class Model
{
private:
	const std::string m_ModePath;
	const std::string m_BaseColorTexPath;

	int m_Width      = 0;
	int m_Height     = 0;
	int m_Components = 0;

	std::vector<ModelRendererProgram::Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;

	std::unique_ptr<Image> m_Image;

public:
	Model(class Device* device, const std::string& modelPath, const std::string& baseColorTexPath);

	inline ModelRendererProgram::Vertex* GetVertices() { return m_Vertices.data(); }
	inline size_t GetVerticesSize() { return m_Vertices.size() * sizeof(m_Vertices[0]); }
	inline size_t GetVerticesCount() { return m_Vertices.size(); }

	inline VkImageView GetImageView() { return m_Image->GetImageView(); }
	inline VkSampler GetImageSampler() { return m_Image->GetSampler(); }
};