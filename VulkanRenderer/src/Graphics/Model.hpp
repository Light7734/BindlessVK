#pragma once

#include "Core/Base.hpp"

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

	inline bool operator==(const Vertex& other) const
	{
		return position == other.position &&
		       color == other.color &&
		       uv == other.uv;
	}
};

namespace std {

template<>
struct hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.position) ^
		         (hash<glm::vec3>()(vertex.color) << 1)) >>
		        1) ^
		       (hash<glm::vec2>()(vertex.uv) << 1);
	}
};
} // namespace std

struct ModelCreateInfo
{
	std::string texturePath;
	std::string modelPath;
};

class Model
{
public:
	Model(ModelCreateInfo& createInfo);
	~Model();

	inline uint32_t GetVerticesSize() const { return m_Vertices.size() * sizeof(Vertex); }
	inline const Vertex* GetVertices() const { return m_Vertices.data(); }

	inline uint32_t GetIndicesSize() const { return m_Indices.size() * sizeof(uint32_t); }
	inline const uint32_t GetIndicesCount() const { return m_Indices.size(); }
	inline const uint32_t* GetIndices() const { return m_Indices.data(); }

private:
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;

	std::unordered_map<Vertex, uint32_t> m_UniqueVertices {};
};
