#pragma once

#include "Framework/Common/Common.hpp"
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
		Scene *scene;
		Window *window;
	};

public:
	CameraController() = default;

	CameraController(Scene *scene, Window *window): window(window), scene(scene)
	{
	}

	void update()
	{
		double delta_time = delta_timer.elapsed_time();
		delta_timer.reset();

		// Keyboard input ; Move Around
		{
			const float delta_y = glfwGetKey(window->get_glfw_handle(), GLFW_KEY_W) ? +1.0f :
			                      glfwGetKey(window->get_glfw_handle(), GLFW_KEY_S) ? -1.0f :
			                                                                          0.0f;

			const float delta_x = glfwGetKey(window->get_glfw_handle(), GLFW_KEY_D) ? +1.0f :
			                      glfwGetKey(window->get_glfw_handle(), GLFW_KEY_A) ? -1.0f :
			                                                                          0.0f;

			move(delta_time, delta_x, delta_y);
		}

		// Mouse input ; Look around
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(window->get_glfw_handle(), &mouse_x, &mouse_y);

			if (glfwGetInputMode(window->get_glfw_handle(), GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
				last_mouse_x = mouse_x;
				last_mouse_y = mouse_y;
				return;
			}

			if (last_mouse_x == std::numeric_limits<double>::max()) {
				last_mouse_x = mouse_x;
				last_mouse_y = mouse_y;
				return;
			}

			const double delta_x = mouse_x - last_mouse_x;
			const double delta_y = mouse_y - last_mouse_y;
			look(delta_x, delta_y);

			last_mouse_x = mouse_x;
			last_mouse_y = mouse_y;
		}
	}

	void on_window_resize(int width, int height)
	{
		scene->group(entt::get<TransformComponent, CameraComponent>)
		    .each([&](TransformComponent &transformComp, CameraComponent &cameraComp) {
			    cameraComp.width = width;
			    cameraComp.aspect_ratio = width / (float)height;
		    });
	}

private:
	void move(float delta_time, float delta_x, float delta_y)
	{
		scene->group(entt::get<TransformComponent, CameraComponent>)
		    .each([&](TransformComponent &transformComp, CameraComponent &cameraComp) {
			    if (delta_x == 0.0 && delta_y == 0.0f) {
				    return;
			    }

			    glm::vec2 speed = cameraComp.speed * delta_time
			                      * glm::normalize(glm::vec2(delta_x, delta_y));

			    transformComp.translation += speed.y * cameraComp.front;
			    transformComp.translation += speed.x
			                                 * glm::normalize(
			                                     glm::cross(cameraComp.front, cameraComp.up)
			                                 );
		    });
	}

	void look(double delta_x, double delta_y)
	{
		scene->group(entt::get<TransformComponent, CameraComponent>)
		    .each([&](TransformComponent &transformComp, CameraComponent &cameraComp) {
			    delta_x *= 0.1;
			    delta_y *= -0.1;

			    cameraComp.yaw += delta_x;
			    cameraComp.pitch += delta_y;

			    cameraComp.pitch = std::clamp(cameraComp.pitch, -89.0, 89.0);

			    cameraComp.front = glm::normalize(glm::vec3(
			        glm::sin(glm::radians(cameraComp.yaw))
			            * glm::cos(glm::radians(cameraComp.pitch)),
			        glm::sin(glm::radians(cameraComp.pitch)),
			        glm::cos(glm::radians(cameraComp.yaw))
			            * glm::cos(glm::radians(cameraComp.pitch))
			    ));
		    });
	}

private:
	Window *window = {};
	Scene *scene = {};
	double last_mouse_x = std::numeric_limits<double>::max();
	double last_mouse_y = std::numeric_limits<double>::max();

	Timer delta_timer = {};
};
