#pragma once

#include "Framework/Core/Common.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/CVar.hpp"
#include "Framework/Utils/Timer.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraController
{
public:
	struct CreateInfo
	{
		Scene* scene;
		Window* window;
	};

public:
	CameraController() = default;

	CameraController(const CreateInfo& info)
	    : m_Window(info.window)
	    , m_Scene(info.scene)
	{
	}

	void Update()
	{
		double deltaTime = m_DeltaTimer.ElapsedTime();
		m_DeltaTimer.Reset();

		// Keyboard input ; Move Around
		{
			const float yDelta = glfwGetKey(m_Window->GetGlfwHandle(), GLFW_KEY_W) ? +1.0f :
			                     glfwGetKey(m_Window->GetGlfwHandle(), GLFW_KEY_S) ? -1.0f :
			                                                                         0.0f;

			const float xDelta = glfwGetKey(m_Window->GetGlfwHandle(), GLFW_KEY_D) ? +1.0f :
			                     glfwGetKey(m_Window->GetGlfwHandle(), GLFW_KEY_A) ? -1.0f :
			                                                                         0.0f;
			Move(deltaTime, xDelta, yDelta);
		}

		// Mouse input ; Look around
		{
			double xPosition, yPosition;
			glfwGetCursorPos(m_Window->GetGlfwHandle(), &xPosition, &yPosition);

			if (m_LastMouseX == std::numeric_limits<double>::max())
			{
				m_LastMouseX = xPosition;
				m_LastMouseY = yPosition;
				return;
			}

			const double xDelta = xPosition - m_LastMouseX;
			const double yDelta = yPosition - m_LastMouseY;
			m_LastMouseX        = xPosition;
			m_LastMouseY        = yPosition;

			Look(xDelta, yDelta);
		}
	}

private:
	void Move(float deltaTime, float xDelta, float yDelta)
	{
		m_Scene->group(entt::get<TransformComponent, CameraComponent>).each([&](TransformComponent& transformComp, CameraComponent& cameraComp) {
			if (xDelta == 0.0 && yDelta == 0.0f)
			{
				return;
			}

			glm::vec2 speed = cameraComp.speed * deltaTime * glm::normalize(glm::vec2(xDelta, yDelta));

			transformComp.translation += speed.y * cameraComp.front;
			transformComp.translation += speed.x * glm::normalize(glm::cross(cameraComp.front, cameraComp.up));
		});
	}

	void Look(double xDelta, double yDelta)
	{
		m_Scene->group(entt::get<TransformComponent, CameraComponent>).each([&](TransformComponent& transformComp, CameraComponent& cameraComp) {
			xDelta *= 0.1;
			yDelta *= -0.1;

			cameraComp.yaw += xDelta;
			cameraComp.pitch += yDelta;

			cameraComp.pitch = std::clamp(cameraComp.pitch, -89.0, 89.0);

			cameraComp.front = glm::normalize(glm::vec3(
			    glm::sin(glm::radians(cameraComp.yaw)) * glm::cos(glm::radians(cameraComp.pitch)),
			    glm::sin(glm::radians(cameraComp.pitch)),
			    glm::cos(glm::radians(cameraComp.yaw)) * glm::cos(glm::radians(cameraComp.pitch))));
		});
	}

private:
	Window* m_Window    = {};
	Scene* m_Scene      = {};
	double m_LastMouseX = std::numeric_limits<double>::max();
	double m_LastMouseY = std::numeric_limits<double>::max();

	Timer m_DeltaTimer = {};
};
