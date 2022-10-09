#pragma once

#include "Core/Base.hpp"
#include "Utils/CVar.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	struct CreateInfo
	{
		glm::vec3 position;
		float speed;
		float nearPlane;
		float farPlane;
		float width;
		float height;
	};

public:
	Camera(const CreateInfo& info)
	    : m_Position(info.position)
	    , m_Up(glm::vec3(0.0f, 0.0f, -1.0f))
	    , m_Front(glm::vec3(0.0f, 1.0f, 0.0))
	    , m_Speed(info.speed)
	    , m_NearPlane(info.nearPlane)
	    , m_FarPlane(info.farPlane)
	    , m_Width(info.width)
	    , m_Height(info.height)
	    , m_FieldOfView(45.0f)
	{
	}

	inline float GetFOV() const
	{
		return glm::radians((float)CVar::Get("CameraFOV"));
	}

	inline glm::mat4 GetProjection() const
	{
		return glm::perspective(m_FieldOfView, m_Width / m_Height, m_NearPlane, m_FarPlane);
	}

	inline glm::mat4 GetView() const
	{
		return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	}

	void Move(float deltaTime, float xDelta, float yDelta)
	{
		if (xDelta == 0.0 && yDelta == 0.0f)
		{
			return;
		}

		glm::vec2 speed = m_Speed * deltaTime * glm::normalize(glm::vec2(xDelta, yDelta));

		m_Position += speed.y * m_Front;
		m_Position += speed.x * glm::normalize(glm::cross(m_Front, m_Up));
	}

	void Look(double xDelta, double yDelta)
	{
		xDelta *= 0.1;
		yDelta *= -0.1;

		m_Yaw += xDelta;
		m_Pitch += yDelta;


		m_Pitch = std::clamp(m_Pitch, -89.0, 89.0);

		m_Front = glm::normalize(glm::vec3(glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch)),
		                                   glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch)),
		                                   glm::sin(glm::radians(m_Pitch))));
	}

private:
	// @TODO: Make this variable hook into ACV_CameraFOV's Change
	float m_FieldOfView = {};

	float m_Width  = {};
	float m_Height = {};

	float m_NearPlane = {};
	float m_FarPlane  = {};

	float m_Speed = {};

	double m_Yaw   = 225.0;
	double m_Pitch = 0.0;

	glm::vec3 m_Position = {};
	glm::vec3 m_Front    = {};
	glm::vec3 m_Up       = {};
};
