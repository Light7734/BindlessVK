#include "BindlessVk/Model/Model.hpp"


#include "BindlessVk/Buffers/Buffer.hpp"

namespace BINDLESSVK_NAMESPACE {

Model::~Model()
{
	ZoneScoped;

	if (nodes.empty())
		return;

	for (auto *node : nodes)
		delete node;
}

auto Model::Vertex::get_attributes() -> arr<vk::VertexInputAttributeDescription, 4>
{
	ZoneScoped;

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
	ZoneScoped;

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
	ZoneScoped;

	auto static const attributes = get_attributes();
	auto static const bindings = get_bindings();

	return vk::PipelineVertexInputStateCreateInfo {
		{},
		bindings,
		attributes,
	};
}

} // namespace BINDLESSVK_NAMESPACE
