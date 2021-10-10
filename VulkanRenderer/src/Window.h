#pragma once

#include "Base.h"

struct GLFWwindow;

class Window
{
private:
	GLFWwindow* m_WindowHandle;

public:
	Window();

	inline GLFWwindow* GetHandle() { return m_WindowHandle; }

	// #todo
	inline bool IsClosed() { return false; }

private:
	void BindGlfwEvents();
};