#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhyisicalDevice,VkDevice newDevice, std::vector<Vertex>* vertices);
	int getVertexCount();
	VkBuffer getVertexBuffer();
	void destoryVertexBuffer();
	~Mesh();
private:
	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory deviceMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void createVertexBuffer(std::vector<Vertex>* vertices);
	uint32_t findMemoryTypeIndex(uint32_t allowedType, VkMemoryPropertyFlags flags);
};

