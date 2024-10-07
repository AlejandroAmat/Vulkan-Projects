#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhyisicalDevice,VkDevice newDevice, std::vector<Vertex>* vertices, VkCommandPool transferPool, VkQueue transferQueue);
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

	void createVertexBuffer(std::vector<Vertex>* vertices, VkCommandPool commandPool, VkQueue queue);
	uint32_t findMemoryTypeIndex(uint32_t allowedType, VkMemoryPropertyFlags flags);
};

