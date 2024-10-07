#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhyisicalDevice,VkDevice newDevice, std::vector<Vertex>* vertices, VkCommandPool transferPool, VkQueue transferQueue, std::vector<uint32_t>* indices);
	int getVertexCount();
	int getIndexCount();
	VkBuffer getIndexBuffer();
	VkBuffer getVertexBuffer();
	void destroyBuffer();
	~Mesh();
private:
	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory deviceMemory;

	int indexCount;
	VkBuffer indexBuffer;
	VkDeviceMemory indexMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void createVertexBuffer(std::vector<Vertex>* vertices, VkCommandPool commandPool, VkQueue queue);
	void createIndexBuffer(std::vector<uint32_t>* ind, VkCommandPool commandPool, VkQueue queue);
	uint32_t findMemoryTypeIndex(uint32_t allowedType, VkMemoryPropertyFlags flags);
};

