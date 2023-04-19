#include "BindlessVk/Model/Model.hpp"

#include "BindlessVk/Buffers/Buffer.hpp"

namespace BINDLESSVK_NAMESPACE {

Model::Model(Model &&other)
{
	*this = std::move(other);
}

Model &Model::operator=(Model &&other)
{
	this->nodes = std::move(other.nodes);
	this->textures = std::move(other.textures);
	this->material_parameters = std::move(other.material_parameters);
	this->vertex_buffer = other.vertex_buffer;
	this->vertex_buffer_region = other.vertex_buffer_region;
	this->index_buffer = other.index_buffer;
	this->debug_name = other.debug_name;

	return *this;
}

Model::~Model()
{
	if (nodes.empty())
		return;

	for (auto *node : nodes)
		delete node;

	delete index_buffer;
	delete vertex_buffer;
}

auto Model::Vertex::get_attributes() -> arr<vk::VertexInputAttributeDescription, 4>
{
	return arr<vk::VertexInputAttributeDescription, 4> {
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
	};
}

auto Model::Vertex::get_bindings() -> arr<vk::VertexInputBindingDescription, 1>
{
	return arr<vk::VertexInputBindingDescription, 1> {
		vk::VertexInputBindingDescription {
		    0u,
		    static_cast<u32>(sizeof(Model::Vertex)),
		    vk::VertexInputRate::eVertex,
		},
	};
}

auto Model::Vertex::get_vertex_input_state() -> vk::PipelineVertexInputStateCreateInfo
{
	auto static const attributes = get_attributes();
	auto static const bindings = get_bindings();

	return vk::PipelineVertexInputStateCreateInfo {
		{},
		bindings,
		attributes,
	};
}

} // namespace BINDLESSVK_NAMESPACE
