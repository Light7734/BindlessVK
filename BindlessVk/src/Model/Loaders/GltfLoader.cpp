#include "BindlessVk/Model/Loaders/GltfLoader.hpp"


#include "BindlessVk/Buffers/Buffer.hpp"
#include "tracy/Tracy.hpp"

namespace BINDLESSVK_NAMESPACE {

auto static mat4_from_f64ptr(f64 const *const ptr) -> mat4
{
	auto mat = mat4 {};
	for (usize i = 0; i < 4 * 4; ++i)
		mat[i] = ptr[i];

	return mat;
}

auto static vec3_from_f64ptr(f64 const *const ptr) -> vec3
{
	auto vec = vec3 {};
	for (usize i = 0; i < 3; ++i)
		vec[i] = ptr[i];

	return vec;
}

void static translate(mat4 &mat, vec3 v)
{
	mat[3 * 4 + 0] += v[0];
	mat[3 * 4 + 1] += v[1];
	mat[3 * 4 + 2] += v[2];
}

void static scale(mat4 &mat, vec3 v)
{
	mat[0 * 4 + 0] *= v[0];
	mat[0 * 4 + 1] *= v[0];
	mat[0 * 4 + 2] *= v[0];
	mat[0 * 4 + 3] *= v[0];

	mat[1 * 4 + 0] *= v[1];
	mat[1 * 4 + 1] *= v[1];
	mat[1 * 4 + 2] *= v[1];
	mat[1 * 4 + 3] *= v[1];

	mat[2 * 4 + 0] *= v[2];
	mat[2 * 4 + 1] *= v[2];
	mat[2 * 4 + 2] *= v[2];
	mat[2 * 4 + 3] *= v[2];
}

GltfLoader::GltfLoader(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    TextureLoader const *const texture_loader,
    FragmentedBuffer *const vertex_buffer,
    FragmentedBuffer *const index_buffer,
    Buffer *const staging_vertex_buffer,
    Buffer *const staging_index_buffer,
    Buffer *const staging_texture_buffer
)
    : vk_context(vk_context)
    , memory_allocator(memory_allocator)
    , texture_loader(texture_loader)
    , vertex_buffer(vertex_buffer)
    , index_buffer(index_buffer)
    , staging_vertex_buffer(staging_vertex_buffer)
    , staging_index_buffer(staging_index_buffer)
    , staging_texture_buffer(staging_texture_buffer)
{
	ZoneScoped;
}

auto GltfLoader::load_from_ascii(str_view const file_path, str_view const debug_name) -> Model
{
	ZoneScoped;

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
	ZoneScoped;

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
		log_wrn("gltf warning -> ", warn);
}

void GltfLoader::load_textures()
{
	ZoneScoped;

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
	ZoneScoped;

	for (auto &material : gltf_model.materials)
	{
		assert_false(
		    material.values.find("baseColorTexture") == material.values.end(),
		    "Material doesn't have required values"
		);

		model.material_parameters.push_back({
		    vec3 { 1.0f },
		    vec3 { 1.0f },
		    vec3 { 1.0f },
		    material.values["baseColorTexture"].TextureIndex(),
		    material.normalTexture.index,
		    material.values["metallicRoughnessTexture"].TextureIndex(),
		});
	}
}

void GltfLoader::stage_mesh_data()
{
	ZoneScoped;

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
	ZoneScoped;

	write_vertex_buffer_to_gpu();
	write_index_buffer_to_gpu();
}

Model::Node *GltfLoader::load_node(const tinygltf::Node &gltf_node, Model::Node *parent_node)
{
	ZoneScoped;

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
	ZoneScoped;

	staging_vertex_buffer->unmap();

	model.vertex_buffer_fragment = vertex_buffer->grab_fragment(
	    vertex_count * sizeof(Model::Vertex)
	);

	vertex_buffer->copy_staging_to_fragment(staging_vertex_buffer, model.vertex_buffer_fragment);
}

void GltfLoader::write_index_buffer_to_gpu()
{
	ZoneScoped;

	staging_index_buffer->unmap();

	model.index_buffer_fragment = index_buffer->grab_fragment(index_count * sizeof(u32));
	index_buffer->copy_staging_to_fragment(staging_index_buffer, model.index_buffer_fragment);
}

void GltfLoader::load_mesh_primitives(const tinygltf::Mesh &gltf_mesh, Model::Node *node)
{
	ZoneScoped;

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
	ZoneScoped;

	auto const *position_buffer = (vec3 const *)
	    get_primitive_attribute_buffer(gltf_primitive, "POSITION");

	auto const *normal_buffer = (vec3 const *)
	    get_primitive_attribute_buffer(gltf_primitive, "NORMAL");

	auto const *tangent_buffer = (vec3 const *)
	    get_primitive_attribute_buffer(gltf_primitive, "TANGENT");

	auto const *uv_buffer = (vec2 const *)(get_primitive_attribute_buffer(
	    gltf_primitive,
	    "TEXCOORD_0"
	));

	auto primitive_vertex_count = get_primitive_vertex_count(gltf_primitive);

	for (auto v = 0; v < primitive_vertex_count; ++v)
	{
		auto const [x, y, z] = tangent_buffer ? tangent_buffer[v] : vec3 { 0.0f };
		auto const mag = sqrt(x * x + y * y + z * z);

		vertex_map[vertex_count] = {
			position_buffer[v],
			normal_buffer ? normal_buffer[v] : vec3 { 0.0f },
			tangent_buffer ? vec3 { x / mag, y / mag, z / mag } : vec3 { 0.0f },
			uv_buffer ? uv_buffer[v] : vec2 { 0.0f },
		};

		++vertex_count;
	}
}

auto GltfLoader::load_mesh_primitive_indices(const tinygltf::Primitive &gltf_primitive) -> u32
{
	ZoneScoped;

	auto const &accessor = gltf_model.accessors[gltf_primitive.indices];
	auto const &buffer_view = gltf_model.bufferViews[accessor.bufferView];
	auto const &buffer = gltf_model.buffers[buffer_view.buffer];
	auto const byte_offset = accessor.byteOffset + buffer_view.byteOffset;

	switch (accessor.componentType)
	{
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
	{
		auto buf = reinterpret_cast<u32 const *>(&buffer.data[byte_offset]);
		for (size_t index = 0; index < accessor.count; index++)
		{
			index_map[index_count] = buf[index] + vertex_count;
			index_count++;
		}
		break;
	}
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
	{
		auto buf = reinterpret_cast<u16 const *>(&buffer.data[byte_offset]);
		for (usize index = 0; index < accessor.count; index++)
		{
			index_map[index_count] = buf[index] + vertex_count;
			index_count++;
		}
		break;
	}
	case TINYGLTF_PARAMETER_TYPE_BYTE:
	{
		auto buf = reinterpret_cast<u8 const *>(&buffer.data[byte_offset]);
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
    tinygltf::Primitive const &gltf_primitive,
    str_view const attribute_name
) -> f32 const *
{
	ZoneScoped;

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
	ZoneScoped;

	auto const &it = gltf_primitive.attributes.find("POSITION");
	if (it == gltf_primitive.attributes.end())
	{
		return 0u;
	}

	// it->second is index of attribute
	return gltf_model.accessors[it->second].count;
}

auto GltfLoader::get_primitive_index_count(tinygltf::Primitive const &gltf_primitive) -> usize
{
	ZoneScoped;

	return gltf_model.accessors[gltf_primitive.indices].count;
}

void GltfLoader::set_initial_node_transform(tinygltf::Node const &gltf_node, Model::Node *node)
{
	ZoneScoped;

	if (!gltf_node.matrix.empty())
		node->transform = mat4_from_f64ptr(gltf_node.matrix.data());

	else
	{
		if (!gltf_node.translation.empty())
			translate(node->transform, vec3_from_f64ptr(gltf_node.translation.data()));

		// We don't support rotation yet
		// if (!gltf_node.rotation.empty())

		if (!gltf_node.scale.empty())
			scale(node->transform, vec3_from_f64ptr(gltf_node.scale.data()));
	}
}

auto GltfLoader::node_has_any_children(tinygltf::Node const &gltf_node) -> bool
{
	ZoneScoped;

	return gltf_node.children.size() > 1;
}

auto GltfLoader::node_has_any_mesh(tinygltf::Node const &gltf_node) -> bool
{
	ZoneScoped;

	return gltf_node.mesh > -1;
}

} // namespace BINDLESSVK_NAMESPACE
