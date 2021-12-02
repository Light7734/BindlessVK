#pragma once

#include "Base.h"

struct GLFWwindow;

struct WindowUserPointer
{
	class Swapchain* swapchain;
	class Renderer* renderer;
};

class Window
{
public:
	Window(uint32_t width, uint32_t height);
	~Window();

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }

	inline GLFWwindow* GetHandle() { return m_WindowHandle; }

	void SetWindowUserPointer(class Renderer* renderer);

	// #todo
	bool IsClosed() const;

	// swapchain resize...
	inline bool WasResized() const { return m_WasResized; }
	inline void ResizeHandled() { m_WasResized = false; }

private:
	GLFWwindow* m_WindowHandle;

	uint32_t m_Width, m_Height;

	bool m_WasResized;

private:
	void BindGlfwEvents();
};