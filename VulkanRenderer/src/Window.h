#pragma once

#include "Base.h"

struct GLFWwindow;

class Window
{
private:
	GLFWwindow* m_WindowHandle;

public:
	Window();
	~Window();

	inline GLFWwindow* GetHandle() { return m_WindowHandle; }

	// #todo
	bool IsClosed() const;

private:
	void BindGlfwEvents();
};