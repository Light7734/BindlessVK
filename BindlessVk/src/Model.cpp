#include "BindlessVk/Model.hpp"

#include "BindlessVk/Buffer.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>

namespace BINDLESSVK_NAMESPACE {

void ModelSystem::init(Device* device, TextureSystem* texture_system)
{
	this->device = device;
	this->texture_system = texture_system;
}

void ModelSystem::reset()
{
	for (auto& [key, val] : models)
	{
		delete val.vertex_buffer;
		delete val.index_buffer;
	}
}

void ModelSystem::load_gltf(const char* name, const char* gltf_path)
{
	tinygltf::Model gltf_model = load_gltf_file(gltf_path);
	Model& model = models[HashStr(name)];

	load_mesh(gltf_model, model);

	load_textures(gltf_model, model);

	load_material_parameters(gltf_model, model);

	create_model_gpu_buffers(name, model);
	free_staging_data();
}

tinygltf::Model ModelSystem::load_gltf_file(const char* gltf_path)
{
	tinygltf::TinyGLTF gltf_context;
	tinygltf::Model gltf_model;

	std::string err, warn;

	BVK_ASSERT(
	    !gltf_context.LoadASCIIFromFile(&gltf_model, &err, &warn, gltf_path),
	    "Failed to load gltf file: {}\nerr: {}",
	    gltf_path,
	    err
	);

	if (!warn.empty())
	{
		BVK_LOG(LogLvl::eWarn, warn);
	}

	return gltf_model;
}

void ModelSystem::load_textures(const tinygltf::Model& gltf_model, Model& model)
{
	for (auto& gltf_image : gltf_model.images)
	{
		model.textures.push_back(texture_system->create_from_gltf(gltf_image));
	}
}

void ModelSystem::load_mesh(const tinygltf::Model& gltf_model, Model& model)
{
	for (auto node_index : gltf_model.scenes[0].nodes)
	{
		auto* node = load_node(gltf_model, gltf_model.nodes[node_index], model, nullptr);
		model.nodes.push_back(node);
	}
}

Model::Node* ModelSystem::load_node(
    const tinygltf::Model& gltf_model,
    const tinygltf::Node& gltf_node,
    Model& model,
    Model::Node* parent_node
)
{
	auto* node = new Model::Node(parent_node);
	set_initial_node_transform(gltf_node, node);

	if (node_has_any_children(gltf_node))
	{
		for (auto child_index : gltf_node.children)
		{
			load_node(gltf_model, gltf_model.nodes[child_index], model, node);
		}
	}

	if (node_has_any_mesh(gltf_node))
	{
		load_mesh_primitives(gltf_model, gltf_model.meshes[gltf_node.mesh], node);
	}

	return node;
}

void ModelSystem::load_mesh_primitives(
    const tinygltf::Model& gltf_model,
    const tinygltf::Mesh& gltf_mesh,
    Model::Node* node
)
{
	for (const auto& primitive : gltf_mesh.primitives)
	{
		u32 first_index = static_cast<u32>(staging_index_buffer.size());
		u32 index_count = load_mesh_primitive_indices(gltf_model, primitive);

		load_mesh_primitive_vertices(gltf_model, primitive);

		node->mesh.push_back({
		    .first_index = first_index,
		    .index_count = index_count,
		    .material_index = primitive.material,
		});
	}
}

void ModelSystem::load_mesh_primitive_vertices(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	const auto* position_buffer = get_mesh_primitive_position_buffer(gltf_model, gltf_primitive);
	const auto* normal_buffer = get_mesh_primitive_normal_buffer(gltf_model, gltf_primitive);
	const auto* tangent_buffer = get_mesh_primitive_tangent_buffer(gltf_model, gltf_primitive);
	const auto* uv_buffer = get_mesh_primitive_uv_buffer(gltf_model, gltf_primitive);
	auto vertex_count = get_mesh_primitive_vertex_count(gltf_model, gltf_primitive);

	for (auto v = 0; v < vertex_count; ++v)
	{
		staging_vertex_buffer.push_back({
		    glm::vec4(glm::make_vec3(&position_buffer[v * 3]), 1.0f),

		    glm::normalize(
		        glm::vec3(normal_buffer ? glm::make_vec3(&normal_buffer[v * 3]) : glm::vec3(0.0f))
		    ),

		    glm::normalize(
		        glm::vec3(tangent_buffer ? glm::make_vec3(&tangent_buffer[v * 3]) : glm::vec3(0.0f))
		    ),

		    uv_buffer ? glm::make_vec2(&uv_buffer[v * 2]) : glm::vec3(0.0f),

		    glm::vec3(1.0),
		});
	}
}

u32 ModelSystem::load_mesh_primitive_indices(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	u32 vertex_start = static_cast<u32>(staging_vertex_buffer.size());

	const auto& accessor = gltf_model.accessors[gltf_primitive.indices];
	const auto& buffer_view = gltf_model.bufferViews[accessor.bufferView];
	const auto& buffer = gltf_model.buffers[buffer_view.buffer];

	switch (accessor.componentType)
	{
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
	{
		const u32* buf =
		    reinterpret_cast<const u32*>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]
		    );
		for (size_t index = 0; index < accessor.count; index++)
		{
			staging_index_buffer.push_back(buf[index] + vertex_start);
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
			staging_index_buffer.push_back(buf[index] + vertex_start);
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
			staging_index_buffer.push_back(buf[index] + vertex_start);
		}
		break;
	}
	}

	return static_cast<u32>(accessor.count);
}


const float* ModelSystem::get_mesh_primitive_position_buffer(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	if (gltf_primitive.attributes.find("POSITION") == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	const auto& accessor = gltf_model.accessors[gltf_primitive.attributes.find("POSITION")->second];
	const auto& view = gltf_model.bufferViews[accessor.bufferView];
	return reinterpret_cast<const float*>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}

const float* ModelSystem::get_mesh_primitive_normal_buffer(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	if (gltf_primitive.attributes.find("NORMAL") == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	const auto& accessor = gltf_model.accessors[gltf_primitive.attributes.find("NORMAL")->second];
	const auto& view = gltf_model.bufferViews[accessor.bufferView];
	return reinterpret_cast<const float*>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}


const float* ModelSystem::get_mesh_primitive_tangent_buffer(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	if (gltf_primitive.attributes.find("TANGENT") == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	const auto& accessor = gltf_model.accessors[gltf_primitive.attributes.find("TANGENT")->second];
	const auto& view = gltf_model.bufferViews[accessor.bufferView];
	return reinterpret_cast<const float*>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}

const float* ModelSystem::get_mesh_primitive_uv_buffer(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	if (gltf_primitive.attributes.find("TEXCOORD_0") == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	const auto& accessor =
	    gltf_model.accessors[gltf_primitive.attributes.find("TEXCOORD_0")->second];
	const auto& view = gltf_model.bufferViews[accessor.bufferView];
	return reinterpret_cast<const float*>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}

size_t ModelSystem::get_mesh_primitive_vertex_count(
    const tinygltf::Model& gltf_model,
    const tinygltf::Primitive& gltf_primitive
)
{
	if (gltf_primitive.attributes.find("POSITION") == gltf_primitive.attributes.end())
	{
		return 0u;
	}

	const auto& accessor = gltf_model.accessors[gltf_primitive.attributes.find("POSITION")->second];
	return accessor.count;
}

void ModelSystem::set_initial_node_transform(const tinygltf::Node& gltf_node, Model::Node* node)
{
	if (gltf_node.translation.size() == 3)
	{
		node->transform = glm::translate(
		    node->transform,
		    glm::vec3(glm::make_vec3(gltf_node.translation.data()))
		);
	}
	if (gltf_node.rotation.size() == 4)
	{
		glm::quat q = glm::make_quat(gltf_node.rotation.data());
		node->transform *= glm::mat4(q);
	}
	if (gltf_node.scale.size() == 3)
	{
		node->transform =
		    glm::scale(node->transform, glm::vec3(glm::make_vec3(gltf_node.scale.data())));
	}
	if (gltf_node.matrix.size() == 16)
	{
		node->transform = glm::make_mat4x4(gltf_node.matrix.data());
	};
}

bool ModelSystem::node_has_any_children(const tinygltf::Node& gltf_node)
{
	return gltf_node.children.size() > 1;
}

bool ModelSystem::node_has_any_mesh(const tinygltf::Node& gltf_node)
{
	return gltf_node.mesh > -1;
}

void ModelSystem::load_material_parameters(const tinygltf::Model& gltf_model, Model& model)
{
	for (auto& material : gltf_model.materials)
	{
		BVK_ASSERT(
		    material.values.find("baseColorTexture") == material.values.end(),
		    "Material doesn't have required values"
		);

		model.material_parameters.push_back({
		    .albedo_factor = glm::vec4(1.0),
		    .albedo_texture_index = material.values.at("baseColorTexture").TextureIndex(),
		});
	}
}

void ModelSystem::create_model_gpu_buffers(const char* name, Model& model)
{
	model.vertex_buffer = new StagingBuffer(
	    fmt::format("{}_model_vb", name).c_str(),
	    device,
	    vk::BufferUsageFlagBits::eVertexBuffer,
	    staging_vertex_buffer.size() * sizeof(Model::Vertex),
	    1u,
	    staging_vertex_buffer.data()
	);

	model.index_buffer = new StagingBuffer(
	    fmt::format("{}_model_ib", name).c_str(),
	    device,
	    vk::BufferUsageFlagBits::eIndexBuffer,
	    staging_index_buffer.size() * sizeof(u32),
	    1u,
	    staging_index_buffer.data()
	);
}

void ModelSystem::free_staging_data()
{
	staging_vertex_buffer.clear();
	staging_index_buffer.clear();
}


} // namespace BINDLESSVK_NAMESPACE
