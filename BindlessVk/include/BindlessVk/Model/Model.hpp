#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Texture/Texture.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

struct Model
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
		glm::vec3 color;

		static constexpr std::array<vk::VertexInputBindingDescription, 1> get_bindings()
		{
			return {
				vk::VertexInputBindingDescription {
				    0u,                                           // binding
				    static_cast<uint32_t>(sizeof(Model::Vertex)), // stride
				    vk::VertexInputRate::eVertex,                 // inputRate
				},
			};
		}

		static constexpr std::array<vk::VertexInputAttributeDescription, 5> get_attributes()
		{
			return {
				vk::VertexInputAttributeDescription {
				    0u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, position),
				},
				vk::VertexInputAttributeDescription {
				    1u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, normal),
				},
				vk::VertexInputAttributeDescription {
				    2u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, tangent),
				},
				vk::VertexInputAttributeDescription {
				    3u,
				    0u,
				    vk::Format::eR32G32Sfloat,
				    offsetof(Model::Vertex, uv),
				},
				vk::VertexInputAttributeDescription {
				    4u,
				    0u,
				    vk::Format::eR32G32B32Sfloat,
				    offsetof(Model::Vertex, color),
				},


			};
			offsetof(Model::Vertex, uv);
		}

		static vk::PipelineVertexInputStateCreateInfo get_vertex_input_state()
		{
			static const auto bindings = get_bindings();
			static const auto attributes = get_attributes();

			return vk::PipelineVertexInputStateCreateInfo {
				{},
				static_cast<uint32_t>(bindings.size()),
				bindings.data(),

				static_cast<uint32_t>(attributes.size()),
				attributes.data(),
			};
		}
	};

	struct Primitive
	{
		uint32_t first_index;
		uint32_t index_count;
		int32_t material_index;
	};

	struct Node
	{
		Node(Node *parent): parent(parent)
		{
		}

		~Node()
		{
			for (auto *node : children)
				delete node;
		}

		Node *parent;
		std::vector<Node *> children;

		std::vector<Primitive> mesh;
		glm::mat4 transform;
	};

	struct MaterialParameters
	{
		glm::vec3 albedo_factor = glm::vec4(1.0f);
		glm::vec3 diffuse_factor = glm::vec4(1.0f);
		glm::vec3 specular_factor = glm::vec4(1.0f);

		int32_t albedo_texture_index;
		int32_t normal_texture_index;
		int32_t metallic_roughness_texture_index;
	};

	/// @todo: Support textures with different samplers
	const char *name;
	std::vector<Texture> textures;
	std::vector<MaterialParameters> material_parameters;
	std::vector<Node *> nodes;

	class Buffer *vertex_buffer;
	class Buffer *index_buffer;
};

} // namespace BINDLESSVK_NAMESPACE
