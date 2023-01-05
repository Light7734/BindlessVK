#include "BindlessVk/Model.hpp"

#include "BindlessVk/Buffer.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

void ModelSystem::init(Device* device)
{
	m_device = device;
}

void ModelSystem::reset()
{
	for (auto& [key, val] : m_models)
	{
		delete val.vertex_buffer;
		delete val.index_buffer;
	}
}

void ModelSystem::load_model(TextureSystem& texture_system, const char* name, const char* gltf_path)
{
	tinygltf::Model input;
	tinygltf::TinyGLTF gltf_context;
	std::string err, warn;

	std::vector<Model::Vertex> vertices;
	std::vector<uint32_t> indices;

	Model& model = m_models[HashStr(name)];

	BVK_ASSERT(
	    !gltf_context.LoadASCIIFromFile(&input, &err, &warn, gltf_path),
	    "Failed to load gltf file: {}@{}\nerr: {}",
	    name,
	    gltf_path,
	    err
	);

	if (!warn.empty())
	{
		BVK_LOG(LogLvl::eWarn, warn);
	}

	// load nodes
	for (size_t node_index : input.scenes[0].nodes)
	{
		load_node(input, model, input.nodes[node_index], nullptr, &vertices, &indices);
	}

	// load textures
	for (tinygltf::Image& image : input.images)
	{
		model.textures.push_back(texture_system.create_from_gltf(&image));
	}

	// load material parameters
	for (tinygltf::Material& material : input.materials)
	{
		BVK_ASSERT(
		    material.values.find("baseColorTexture") == material.values.end(),
		    "Material doesn't have required values"
		);

		model.material_parameters.push_back({
		    .albedo_factor        = glm::vec4(1.0),
		    .albedo_texture_index = material.values["baseColorTexture"].TextureIndex(),
		});
	}

	model.vertex_buffer = new StagingBuffer(
	    name,
	    m_device,
	    vk::BufferUsageFlagBits::eVertexBuffer,
	    vertices.size() * sizeof(Model::Vertex),
	    1u,
	    vertices.data()
	);

	m_device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (uint64_t)(VkBuffer)(*model.vertex_buffer->get_buffer()),
	    fmt::format("{}_model_vb", name).c_str(),
	});

	model.index_buffer = new StagingBuffer(
	    name,
	    m_device,
	    vk::BufferUsageFlagBits::eIndexBuffer,
	    indices.size() * sizeof(uint32_t),
	    1u,
	    indices.data()
	);

	m_device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (uint64_t)(VkBuffer)(*model.index_buffer->get_buffer()),
	    fmt::format("{}_model_ib", name).c_str(),
	});
}

void ModelSystem::load_node(
    tinygltf::Model& input,
    Model& model,
    const tinygltf::Node& input_node,
    Model::Node* parent,
    std::vector<Model::Vertex>* vertices,
    std::vector<uint32_t>* indices
)
{
	Model::Node* node = new Model::Node {};

	if (parent)
	{
		parent->children.push_back(node);
	}
	else
	{
		model.nodes.push_back(node);
	}

	node->parent = parent;

	if (input_node.translation.size() == 3)
	{
		node->transform = glm::translate(
		    node->transform,
		    glm::vec3(glm::make_vec3(input_node.translation.data()))
		);
	}
	if (input_node.rotation.size() == 4)
	{
		glm::quat q = glm::make_quat(input_node.rotation.data());
		node->transform *= glm::mat4(q);
	}
	if (input_node.scale.size() == 3)
	{
		node->transform =
		    glm::scale(node->transform, glm::vec3(glm::make_vec3(input_node.scale.data())));
	}
	if (input_node.matrix.size() == 16)
	{
		node->transform = glm::make_mat4x4(input_node.matrix.data());
	};

	if (input_node.children.size() > 0)
	{
		for (int32_t childIndex : input_node.children)
		{
			load_node(input, model, input.nodes[childIndex], node, vertices, indices);
		}
	}

	if (input_node.mesh > -1)
	{
		tinygltf::Mesh mesh = input.meshes[input_node.mesh];

		for (const tinygltf::Primitive& primitive : mesh.primitives)
		{
			uint32_t first_index  = static_cast<uint32_t>(indices->size());
			uint32_t vertex_start = static_cast<uint32_t>(vertices->size());
			uint32_t index_count  = 0ul;

			// Vertices
			{
				const float* position_buffer   = nullptr;
				const float* normal_buffer     = nullptr;
				const float* tangent_buffer    = nullptr;
				const float* tex_coords_buffer = nullptr;
				size_t vertex_count            = 0ul;

				if (primitive.attributes.find("POSITION") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor =
					    input.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];

					position_buffer = reinterpret_cast<const float*>(
					    &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
					);
					vertex_count = accessor.count;
				}

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor =
					    input.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];

					normal_buffer = reinterpret_cast<const float*>(
					    &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
					);
				}

				if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor =
					    input.accessors[primitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tangent_buffer                   = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
                    );
				}
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor =
					    input.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tex_coords_buffer                = reinterpret_cast<const float*>(
                        &(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
                    );
				}

				for (size_t v = 0; v < vertex_count; ++v)
				{
					vertices->push_back({
					    .position = glm::vec4(glm::make_vec3(&position_buffer[v * 3]), 1.0f),
					    .normal   = glm::normalize(glm::vec3(
                            normal_buffer ? glm::make_vec3(&normal_buffer[v * 3]) : glm::vec3(0.0f)
                        )),
					    .tangent  = glm::normalize(glm::vec3(
                            tangent_buffer ? glm::make_vec3(&tangent_buffer[v * 3]) :
					                         glm::vec3(0.0f)
                        )),
					    .uv       = tex_coords_buffer ? glm::make_vec2(&tex_coords_buffer[v * 2]) :
					                                    glm::vec3(0.0f),
					    .color    = glm::vec3(1.0),
					});
				}
			}

			// Indices
			{
				const tinygltf::Accessor& accessor = input.accessors[primitive.indices];

				const tinygltf::BufferView& buffer_view = input.bufferViews[accessor.bufferView];

				const tinygltf::Buffer& buffer = input.buffers[buffer_view.buffer];

				index_count += static_cast<uint32_t>(accessor.count);

				switch (accessor.componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				{
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(
					    &buffer.data[accessor.byteOffset + buffer_view.byteOffset]
					);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indices->push_back(buf[index] + vertex_start);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(
					    &buffer.data[accessor.byteOffset + buffer_view.byteOffset]
					);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indices->push_back(buf[index] + vertex_start);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_BYTE:
				{
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(
					    &buffer.data[accessor.byteOffset + buffer_view.byteOffset]
					);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indices->push_back(buf[index] + vertex_start);
					}
					break;
				}
				}
			}

			node->mesh.push_back({
			    .first_index    = first_index,
			    .index_count    = index_count,
			    .material_index = primitive.material,
			});
		}
	}
}
} // namespace BINDLESSVK_NAMESPACE
