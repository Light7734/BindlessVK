#pragma once

#include "BindlessVk/MaterialSystem.hpp"
#include "BindlessVk/ModelLoader.hpp"
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

	inline glm::mat4 GetTransform() const
	{
		return glm::translate(translation) * glm::rotate(rotation.x, glm::vec3(1.0f, 0.0f, 0.0f))
		       * glm::rotate(rotation.y, glm::vec3(0.0f, 1.0f, 0.0f))
		       * glm::rotate(rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(scale);
	}

	inline operator glm::mat4() const
	{
		return GetTransform();
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

	inline auto get_projection() const -> glm::mat4
	{
		return glm::perspective(fov, aspect_ratio, near_plane, far_plane);
	}

	inline auto get_view(const glm::vec3 &position) const -> glm::mat4
	{
		return glm::lookAt(position, position + front, up);
	}


	inline auto to_str(glm::vec3 const &position = { 0.0, 0.0, 0.0 }) const -> str
	{
		auto const proj = get_projection();
		auto const view = get_view(position);

		return fmt::
		    format(
		        "\nproj: [ {}, {}, {}, {} ]\nproj: [{}, {}, {}, {}]\nproj: [{}, {}, {}, {}]\nproj: [{}, {}, {}, {}]\n-----\n"
		        "view: [ {}, {}, {}, {} ]\nview: [{}, {}, {}, {}]\nview: [{}, {}, {}, {}]\nview: [{}, {}, {}, {}]",
		        proj[0][0],
		        proj[0][1],
		        proj[0][2],
		        proj[0][3],

		        proj[1][0],
		        proj[1][1],
		        proj[1][2],
		        proj[1][3],

		        proj[2][0],
		        proj[2][1],
		        proj[2][2],
		        proj[2][3],

		        proj[3][0],
		        proj[3][1],
		        proj[3][2],
		        proj[3][3],

		        view[0][0],
		        view[0][1],
		        view[0][2],
		        view[0][3],

		        view[1][0],
		        view[1][1],
		        view[1][2],
		        view[1][3],

		        view[2][0],
		        view[2][1],
		        view[2][2],
		        view[2][3],

		        view[3][0],
		        view[3][1],
		        view[3][2],
		        view[3][3]
		    );
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

// @todo:
struct LightComponent
{
	LightComponent(const LightComponent &) = default;
	LightComponent(uint32_t b): a(b)
	{
	}
	uint32_t a;
};
