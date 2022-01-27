#pragma once

#include "Core/Base.hpp"

struct GLFWwindow;


struct WindowSpecs
{
	std::string title;
	uint32_t width, height;
	bool resizable, floating;
};

class Window
{
public:
	Window(WindowSpecs& specs);
	~Window();

	bool ShouldClose();

private:
	GLFWwindow* m_GlfwWindowHandle = nullptr;
	WindowSpecs m_Specs;


	void BindCallbacks();
};
