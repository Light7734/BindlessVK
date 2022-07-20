#pragma once

#include "Core/Base.hpp"
#include "Core/UUID.hpp"
#include "Graphics/Texture.hpp"

#include <volk.h>

#include <glm/glm.hpp>

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

struct RenderableCreateInfo
{
	std::string modelPath;
	std::vector<std::shared_ptr<Texture>> textures;
};

class Renderable
{
	friend class Pipeline;

public:
	Renderable(RenderableCreateInfo& createInfo);
	~Renderable();

	inline uint32_t GetVerticesSize() const { return m_Vertices.size() * sizeof(Vertex); }
	inline std::vector<Vertex>& GetVertices() { return m_Vertices; }

	inline uint32_t GetIndicesSize() const { return m_Indices.size() * sizeof(uint32_t); }
	inline const uint32_t GetIndicesCount() const { return m_Indices.size(); }
	inline const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

	inline const std::vector<std::shared_ptr<Texture>> GetTextures() const { return m_Textures; }

private:
	std::vector<Vertex> m_Vertices  = {};
	std::vector<uint32_t> m_Indices = {};

	std::vector<std::shared_ptr<Texture>> m_Textures = {};
	UUID m_Id;
};
