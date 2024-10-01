#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include "utilities.h"

class VulkanRender
{
public :

	VulkanRender();

	int init(GLFWwindow* newWindow);
	void cleanUp();

	~VulkanRender();

private:

	GLFWwindow* window;
	VkInstance instance;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	}mainDevice;
	VkQueue graphicsQueue;

	//get functions
	void getPhysicalDevice();

	//create Functions
	void createInstance();
	void createLogicalDevice();
	//support
	bool checkInstanceExtensionSupport(std::vector<const char*>* extensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);


	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
};

