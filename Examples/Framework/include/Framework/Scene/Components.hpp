#pragma once

#include "BindlessVk/Material/MaterialSystem.hpp"
#include "BindlessVk/Model/Model.hpp"
#include "Framework/Common/Common.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

struct TransformComponent
{
	glm::vec3 translation;
	glm::vec3 scale;
	glm::vec3 rotation;

	TransformComponent(const TransformComponent &) = default;

	TransformComponent(
	    const glm::vec3 &translation,
	    const glm::vec3 &scale = glm::vec3(1.0),
	    const glm::vec3 &rotation = glm::vec3(0.0f)
	)
	    : translation(translation)
	    , scale(scale)
	    , rotation(rotation)
	{
	}

	auto get_transform() const
	{
		return glm::translate(translation) * glm::rotate(rotation.x, glm::vec3(1.0f, 0.0f, 0.0f))
		       * glm::rotate(rotation.y, glm::vec3(0.0f, 1.0f, 0.0f))
		       * glm::rotate(rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(scale);
	}

	operator glm::mat4() const
	{
		return get_transform();
	}
};

struct StaticMeshRendererComponent
{
	StaticMeshRendererComponent(const StaticMeshRendererComponent &) = default;

	StaticMeshRendererComponent(bvk::Material *material, bvk::Model *model)
	    : material(material)
	    , model(model)
	{
	}

	bvk::Material *material;
	bvk::Model *model;
};


struct SkyboxComponent
{
	SkyboxComponent(const SkyboxComponent &) = default;

	SkyboxComponent(bvk::Material *material, bvk::Texture *texture, bvk::Model *model)
	    : material(material)
	    , texture(texture)
	    , model(model)
	{
	}

	bvk::Material *material;
	bvk::Texture *texture;
	bvk::Model *model;
};


struct CameraComponent
{
	CameraComponent(const CameraComponent &) = default;

	CameraComponent(
	    float fov,
	    float width,
	    float aspect_ratio,
	    float near_plane,
	    float far_plane,
	    double yaw,
	    double pitch,
	    glm::vec3 front,
	    glm::vec3 up,
	    float speed
	)

	    : fov(fov)
	    , width(width)
	    , aspect_ratio(aspect_ratio)
	    , near_plane(near_plane)
	    , far_plane(far_plane)
	    , yaw(yaw)
	    , pitch(pitch)
	    , front(front)
	    , up(up)
	    , speed(speed)

	{
	}

	auto get_projection() const
	{
		return glm::perspective(fov, aspect_ratio, near_plane, far_plane);
	}

	auto get_view(const glm::vec3 &position) const
	{
		return glm::lookAt(position, position + front, up);
	}

	float fov;

	float width;
	float aspect_ratio;

	float near_plane;
	float far_plane;

	double yaw;
	double pitch;

	glm::vec3 front;
	glm::vec3 up;

	float speed;
};

struct LightComponent
{
	LightComponent(const LightComponent &) = default;

	LightComponent(uint32_t b): a(b)
	{
	}

	// components can't have no size
	uint32_t a;
};

struct CullViewComponent
{
	CullViewComponent(CullViewComponent const &) = default;

	CullViewComponent(bvk::ShaderPipeline *pipeline): pipeline(pipeline)
	{
	}

	bvk::ShaderPipeline *pipeline;
};
