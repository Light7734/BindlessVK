#include "BindlessVk/Model/Loaders/GltfLoader.hpp"

#include "BindlessVk/Buffer.hpp"

#include <fmt/format.h>

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

auto GltfLoader::load_from_ascii(str_view const file_path, str_view const debug_name) -> Model
{
	model.debug_name = debug_name;

	load_gltf_model_from_ascii(file_path);

	load_textures();

	load_material_parameters();

	stage_mesh_data();
	write_mesh_data_to_gpu();

	return std::move(model);
}

void GltfLoader::load_gltf_model_from_ascii(str_view const file_path)
{
	tinygltf::TinyGLTF gltf_context;
	str err, warn;

	assert_true(
	    gltf_context.LoadASCIIFromFile(&gltf_model, &err, &warn, file_path.data()),
	    "Failed to load gltf file: \nname: {}\npath: {}\nerr: {}",
	    model.debug_name,
	    file_path,
	    err
	);

	if (!warn.empty())
		vk_context->log(LogLvl::eWarn, "gltf warning -> ", warn);
}

void GltfLoader::load_textures()
{
	model.textures.reserve(gltf_model.images.size());

	for (u32 i = 0; auto &image : gltf_model.images)
	{
		model.textures.push_back(texture_loader->load_from_binary(
		    &image.image[0],
		    image.width,
		    image.height,
		    image.image.size(),
		    Texture::Type::e2D,
		    staging_texture_buffer,
		    vk::ImageLayout::eShaderReadOnlyOptimal,
		    image.uri
		));
	}
}

void GltfLoader::load_material_parameters()
{
	auto t = gltf_model.materials.begin();
	for (auto &material : gltf_model.materials)
	{
		assert_false(
		    material.values.find("baseColorTexture") == material.values.end(),
		    "Material doesn't have required values"
		);

		model.material_parameters.push_back({
		    Vec3f(1.0),
		    Vec3f(1.0),
		    Vec3f(1.0),
		    material.values["baseColorTexture"].TextureIndex(),
		    material.normalTexture.index,
		    material.values["metallicRoughnessTexture"].TextureIndex(),
		});
	}
}

void GltfLoader::stage_mesh_data()
{
	vertex_map = static_cast<Model::Vertex *>(staging_vertex_buffer->map_block(0));
	index_map = static_cast<u32 *>(staging_index_buffer->map_block(0));

	for (auto gltf_node_index : gltf_model.scenes[0].nodes)
	{
		auto const &gltf_node = gltf_model.nodes[gltf_node_index];
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
			auto const &child_gltf_node = gltf_model.nodes[gltf_node_index];
			load_node(child_gltf_node, node);
		}
	}

	if (node_has_any_mesh(gltf_node))
	{
		auto const &gltf_mesh = gltf_model.meshes[gltf_node.mesh];
		load_mesh_primitives(gltf_mesh, node);
	}

	return node;
}

void GltfLoader::write_vertex_buffer_to_gpu()
{
	model.vertex_buffer = new Buffer(
	    vk_context,
	    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eAutoPreferDevice,
	    },
	    vertex_count * sizeof(Model::Vertex),
	    1,
	    fmt::format("{}_model_vb", model.debug_name)
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
	    vk_context,
	    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eAutoPreferDevice,
	    },
	    index_count * sizeof(u32),
	    1,
	    fmt::format("{}_model_ib", model.debug_name)
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
	for (auto const &gltf_primitive : gltf_mesh.primitives)
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
	auto const *position_buffer = get_primitive_attribute_buffer(gltf_primitive, "POSITION");
	auto const *normal_buffer = get_primitive_attribute_buffer(gltf_primitive, "NORMAL");
	auto const *tangent_buffer = get_primitive_attribute_buffer(gltf_primitive, "TANGENT");
	auto const *uv_buffer = get_primitive_attribute_buffer(gltf_primitive, "TEXCOORD_0");

	auto primitive_vertex_count = get_primitive_vertex_count(gltf_primitive);

	for (auto v = 0; v < primitive_vertex_count; ++v)
	{
		vertex_map[vertex_count] = {
			Vec3f(&position_buffer[v * 3]),
			normal_buffer ? Vec3f(normal_buffer[v * 3]) : Vec3f(0.0f),
			tangent_buffer ? Vec3f(&tangent_buffer[v * 3]).unit() : Vec3f(0.0f),
			uv_buffer ? Vec2f(&uv_buffer[v * 2]) : Vec2f(0.0f),
			Vec3f(1.0),
		};

		vertex_count++;
	}
}

auto GltfLoader::load_mesh_primitive_indices(const tinygltf::Primitive &gltf_primitive) -> u32
{
	auto const &accessor = gltf_model.accessors[gltf_primitive.indices];
	auto const &buffer_view = gltf_model.bufferViews[accessor.bufferView];
	auto const &buffer = gltf_model.buffers[buffer_view.buffer];

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
    str_view const attribute_name
) -> const f32 *
{
	auto const &it = gltf_primitive.attributes.find(attribute_name.data());
	if (it == gltf_primitive.attributes.end())
	{
		return nullptr;
	}

	auto const &accessor = gltf_model.accessors[it->second];
	auto const &view = gltf_model.bufferViews[accessor.bufferView];

	return reinterpret_cast<const f32 *>(
	    &(gltf_model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset])
	);
}

auto GltfLoader::get_primitive_vertex_count(const tinygltf::Primitive &gltf_primitive) -> usize
{
	auto const &it = gltf_primitive.attributes.find("POSITION");
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
	if (!gltf_node.matrix.empty())
		node->transform = Mat4f(gltf_node.matrix.data());

	else
	{
		if (!gltf_node.translation.empty())
			node->transform.translate(Vec3f(gltf_node.translation.data()));

		if (!gltf_node.rotation.empty())
			node->transform *= Mat4f(Vec4f(gltf_node.rotation.data()));

		if (!gltf_node.scale.empty())
			node->transform.scale(Vec3f(gltf_node.scale.data()));
	}
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
