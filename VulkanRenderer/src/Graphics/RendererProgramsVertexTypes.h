#pragma once

#include "Core/Base.h"

#include <glm/glm.hpp>

struct RainbowRectVertex
{
	glm::vec2 position;
	glm::vec3 color;

	constexpr static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription
		{
			.binding = 0u,
			.stride = sizeof(RainbowRectVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		return bindingDescription;
	}

	constexpr static std::array<VkVertexInputAttributeDescription, 2> GetAttributesDescription()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributesDescription;

		attributesDescription[0] =
		{
			.location = 0u,
			.binding = 0u,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(RainbowRectVertex, position),
		};

		attributesDescription[1] =
		{
			.location = 1u,
			.binding = 0u,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(RainbowRectVertex, color),
		};

		return attributesDescription;
	}
};