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

	TransformComponent(TransformComponent const &) = default;

	TransformComponent(
	    glm::vec3 const &translation,
	    glm::vec3 const &scale = glm::vec3(1.0),
	    glm::vec3 const &rotation = glm::vec3(0.0f)
	)
	    : translation(translation)
	    , scale(scale)
	    , rotation(rotation)
	{
	}

	auto get_transform() const
	{
		return glm::translate(translation) * glm::scale(scale);
	}

	operator glm::mat4() const
	{
		return get_transform();
	}
};

struct StaticMeshComponent
{
	StaticMeshComponent(StaticMeshComponent const &) = default;

	StaticMeshComponent(bvk::Material *material, bvk::Model *model)
	    : material(material)
	    , model(model)
	{
	}

	bvk::Material *material;
	bvk::Model *model;
};


struct SkyboxComponent
{
	SkyboxComponent(SkyboxComponent const &) = default;

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
	CameraComponent(CameraComponent const &) = default;

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

	auto get_view(glm::vec3 const &translation) const
	{
		return glm::lookAt(translation, translation + front, up);
	}

	auto get_view_projection(glm::vec3 const &translation) const
	{
		return get_projection() * get_view(translation);
	}

	f32 fov;

	f32 width;
	f32 aspect_ratio;

	f32 near_plane;
	f32 far_plane;

	f64 yaw;
	f64 pitch;

	glm::vec3 front;
	glm::vec3 up;

	f32 speed;
};

struct DirectionalLightComponent
{
	DirectionalLightComponent(DirectionalLightComponent const &) = default;

	DirectionalLightComponent(
	    glm::vec4 const &direction,
	    glm::vec4 const &ambient,
	    glm::vec4 const &diffuse,
	    glm::vec4 const &specular
	)
	    : direction(direction)
	    , ambient(ambient)
	    , diffuse(diffuse)
	    , specular(specular)
	{
	}

	glm::vec4 direction;

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct PointLightComponent
{
	PointLightComponent(PointLightComponent const &) = default;

	PointLightComponent(
	    f32 constant,
	    f32 linear,
	    f32 quadratic,
	    glm::vec4 const &ambient,
	    glm::vec4 const &diffuse,
	    glm::vec4 const &specular
	)
	    : constant(constant)
	    , linear(linear)
	    , quadratic(quadratic)
	    , ambient(ambient)
	    , diffuse(diffuse)
	    , specular(specular)
	{
	}

	f32 constant;
	f32 linear;
	f32 quadratic;
	f32 _0;

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct CullViewComponent
{
	CullViewComponent(CullViewComponent const &) = default;

	CullViewComponent(bvk::ShaderPipeline *pipeline): pipeline(pipeline)
	{
	}

	bvk::ShaderPipeline *pipeline;
};
