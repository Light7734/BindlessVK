#pragma once

#include "Core/Base.hpp"
#include "Graphics/MaterialSystem.hpp"
#include "Graphics/Model.hpp"

#include <entt/entt.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>

struct TransformComponent
{
	glm::vec3 translation;
	glm::vec3 scale;
	glm::vec3 rotation;

	TransformComponent(const TransformComponent&) = default;

	TransformComponent(const glm::vec3& _translation, const glm::vec3& _scale = glm::vec3(1.0), const glm::vec3& _rotation = glm::vec3(0.0f))
	    : translation(_translation)
	    , scale(_scale)
	    , rotation(_rotation)
	{
	}

	inline glm::mat4 GetTransform() const
	{
		return glm::translate(translation) *
		       glm::rotate(rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
		       glm::rotate(rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
		       glm::rotate(rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) *
		       glm::scale(scale);
	}

	inline operator glm::mat4() const { return GetTransform(); }
};

struct StaticMeshRendererComponent
{
	StaticMeshRendererComponent(const StaticMeshRendererComponent&) = default;

	StaticMeshRendererComponent(Material* _material, Model* _model)
	    : material(_material)
	    , model(_model)
	{
	}

	Material* material;
	Model* model;
};
