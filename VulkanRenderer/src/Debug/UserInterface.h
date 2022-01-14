#pragma

#include "Core/Base.h"

#include <volk.h>

struct GLFWwindow;

class UserInterface
{
private:
	VkDescriptorPool m_DescriptorPool;

public:
	UserInterface(GLFWwindow* window, class Device* devicem, VkRenderPass renderPass);
	~UserInterface();
};
