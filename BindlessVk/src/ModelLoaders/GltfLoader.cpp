#include "BindlessVk/ModelLoaders/GltfLoader.hpp"

#include "BindlessVk/Buffer.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BINDLESSVK_NAMESPACE {

GltfLoader::GltfLoader(
    VkContext const *const vk_context,
    TextureLoader const *const texture_loader,
    Buffer *const staging_vertex_buffer,
    Buffer *const staging_index_buffer,
    Buffer *const staging_texture_buffer
)
    : vk_context(vk_context)
    , texture_loader(texture_loader)
    , staging_vertex_buffer(staging_vertex_buffer)
    , staging_index_buffer(staging_index_buffer)
    , staging_texture_buffer(staging_texture_buffer)
{
}

auto GltfLoader::load_from_ascii(c_str const debug_name, c_str const file_path) -> Model
{
	model = { debug_name };

	load_gltf_model_from_ascii(file_path);

	load_textures();

	load_material_parameters();

	stage_mesh_data();
	write_mesh_data_to_gpu();

	return model;
}

void GltfLoader::load_gltf_model_from_ascii(c_str const file_path)
{
	tinygltf::TinyGLTF gltf_context;
	std::string err, warn;

	assert_true(
	    gltf_context.LoadASCIIFromFile(&gltf_model, &err, &warn, file_path),
	    "Failed to load gltf file: \nname: {}\npath: {}\nerr: {}",
	    model.name,
	    file_path,
	    err
	);

	if (!warn.empty())
	{
		vk_context->log(LogLvl::eWarn, "gltf warning -> ", warn);
	}
}

void GltfLoader::load_textures()
{
	model.textures.reserve(gltf_model.images.size());

	for (auto &image : gltf_model.images)
	{
		model.textures.push_back(texture_loader->load_from_binary(
		    image.uri.c_str(),
		    &image.image[0],
		    image.width,
		    image.height,
		    image.image.size(),
		    Texture::Type::e2D,
		    staging_texture_buffer,
		    vk::ImageLayout::eShaderReadOnlyOptimal
		));
	}
}

void GltfLoader::load_material_parameters()
{
	for (auto &material : gltf_model.materials)
	{
		assert_false(
		    material.values.find("baseColorTexture") == material.values.end(),
		    "Material doesn't have required values"
		);

		model.material_parameters.push_back({
		    .albedo_factor = glm::vec4(1.0),
		    .albedo_texture_index = material.values.at("baseColorTexture").TextureIndex(),
		});
	}
}

void GltfLoader::stage_mesh_data()
{
	vertex_map = static_cast<Model::Vertex *>(staging_vertex_buffer->map_block(0));
	index_map = static_cast<u32 *>(staging_index_buffer->map_block(0));

	for (auto gltf_node_index : gltf_model.scenes[0].nodes)
	{
		const auto &gltf_node = gltf_model.nodes[gltf_node_index];
		model.nodes.push_back(load_node(gltf_node, nullptr));
	}
}

void GltfLoader::write_mesh_data_to_gpu()
{
	write_vertex_buffer_to_gpu();
	write_index_buffer_to_gpu();
}

Model::Node *GltfLoader::load_node(const tinygltf::Node &gltf_node, Model::Node *parent_node)
{
	auto *node = new Model::Node(parent_node);
	set_initial_node_transform(gltf_node, node);

	if (node_has_any_children(gltf_node))
	{
		for (auto gltf_node_index : gltf_node.children)
		{
			const auto &child_gltf_node = gltf_model.nodes[gltf_node_index];
			load_node(child_gltf_node, node);
		}
	}

	if (node_has_any_mesh(gltf_node))
	{
		const auto &gltf_mesh = gltf_model.meshes[gltf_node.mesh];
		load_mesh_primitives(gltf_mesh, node);
	}

	return node;
}

void GltfLoader::write_vertex_buffer_to_gpu()
{
	model.vertex_buffer = new Buffer(
	    fmt::format("{}_model_vb", model.name).c_str(),
	    vk_context,
	    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eAutoPreferDevice,
	    },
	    vertex_count * sizeof(Model::Vertex),
	    1
	);

	staging_vertex_buffer->unmap();
	model.vertex_buffer->write_buffer(
	    *staging_vertex_buffer,
	    {
	        0u,
	        0u,
	        vertex_count * sizeof(Model::Vertex),
	    }
	);
}

void GltfLoader::write_index_buffer_to_gpu()
{
	model.index_buffer = new Buffer(
	    fmt::format("{}_model_ib", model.name).c_str(),
	    vk_context,
	    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eAutoPreferDevice,
	    },
	    index_count * sizeof(u32),
	    1
	);

	staging_index_buffer->unmap();
	model.index_buffer->write_buffer(
	    *staging_index_buffer,
	    {
	        0u,
	        0u,
	        index_count * sizeof(u32),
	    }
	);

	vk_context->get_device().waitIdle();
}

void GltfLoader::load_mesh_primitives(const tinygltf::Mesh &gltf_mesh, Model::Node *node)
{
	for (const auto &gltf_primitive : gltf_mesh.primitives)
	{
		auto first_index = index_count;
		auto index_count = get_primitive_index_count(gltf_primitive);

		load_mesh_primitive_indices(gltf_primitive);
		load_mesh_primitive_vertices(gltf_primitive);

		node->mesh.push_back({
		    static_cast<u32>(first_index),
		    static_cast<u32>(index_count),
		    gltf_primitive.material,
		});
	}
}

void GltfLoader::load_mesh_primitive_vertices(const tinygltf::Primitive &gltf_primitive)
{
	const auto *position_buffer = get_primitive_attribute_buffer(gltf_primitive, "POSITION");
	const auto *normal_buffer = get_primitive_attribute_buffer(gltf_primitive, "NORMAL");
	const auto *tangent_buffer = get_primitive_attribute_buffer(gltf_primitive, "TANGENT");
	const auto *uv_buffer = get_primitive_attribute_buffer(gltf_primitive, "TEXCOORD_0");

	auto primitive_vertex_count = get_primitive_vertex_count(gltf_primitive);

	for (auto v = 0; v < primitive_vertex_count; ++v)
	{
		vertex_map[vertex_count] = {
			glm::vec4(glm::make_vec3(&position_buffer[v * 3]), 1.0f),

			glm::normalize(
			    glm::vec3(normal_buffer ? glm::make_vec3(&normal_buffer[v * 3]) : glm::vec3(0.0f))
			),

			glm::normalize(
			    glm::vec3(tangent_buffer ? glm::make_vec3(&tangent_buffer[v * 3]) : glm::vec3(0.0f))
			),

			uv_buffer ? glm::make_vec2(&uv_buffer[v * 2]) : glm::vec3(0.0f),

			glm::vec3(1.0),
		};

		vertex_count++;
	}
}

auto GltfLoader::load_mesh_primitive_indices(const tinygltf::Primitive &gltf_primitive) -> u32
{
	const auto &accessor = gltf_model.accessors[gltf_primitive.indices];
	const auto &buffer_view = gltf_model.bufferViews[accessor.bufferView];
	const auto &buffer = gltf_model.buffers[buffer_view.buffer];

	switch (accessor.componentType)
	{
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
	{
		const u32 *buf =
		    reinterpret_cast<const u32 *>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]
		    );
		for (size_t index = 0; index < accessor.count; index++)
		{
			index_map[index_count] = buf[index] + vertex_count;
			index_count++;
		}
		break;
	}
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
	{
		const u16 *buf =
		    reinterpret_cast<const u16 *>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]
		    );
		for (usize index = 0; index < accessor.count; index++)
		{
			index_map[index_count] = buf[index] + vertex_count;
			index_count++;
		}
		break;
	}
	case TINYGLTF_PARAMETER_TYPE_BYTE:
	{
		const u8 *buf =
		    reinterpret_cast<const u8 *>(&buffer.data[accessor.byteOffset + buffer_view.byteOffset]
		    );
		for (usize index = 0; index < accessor.count; index++)
		{
			index_map[index_count] = buf[index] + vertex_count;
			index_count++;
		}
		break;
	}
	}

	return static_cast<u32>(accessor.count);
}

auto GltfLoader::get_primitive_attribute_buffer(
    const tinygltf::Primitive &gltf_primitive,
    c_str const attribute_name
) -> const f32 *
{
	const auto &it = gltf_primitive.attributes.find(attribute_name);
	if (it == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	const auto &accessor = gltf_model.accessors[it->second];
	const auto &view = gltf_model.bufferViews[accessor.bufferView];

	return reinterpret_cast<const f32 *>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}

auto GltfLoader::get_primitive_vertex_count(const tinygltf::Primitive &gltf_primitive) -> usize
{
	const auto &it = gltf_primitive.attributes.find("POSITION");
	if (it == gltf_primitive.attributes.end())
	{
		return 0u;
	}

	// it->second is index of attribute
	return gltf_model.accessors[it->second].count;
}

auto GltfLoader::get_primitive_index_count(const tinygltf::Primitive &gltf_primitive) -> usize
{
	return gltf_model.accessors[gltf_primitive.indices].count;
}

void GltfLoader::set_initial_node_transform(const tinygltf::Node &gltf_node, Model::Node *node)
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

auto GltfLoader::node_has_any_children(const tinygltf::Node &gltf_node) -> bool
{
	return gltf_node.children.size() > 1;
}

auto GltfLoader::node_has_any_mesh(const tinygltf::Node &gltf_node) -> bool
{
	return gltf_node.mesh > -1;
}

} // namespace BINDLESSVK_NAMESPACE
