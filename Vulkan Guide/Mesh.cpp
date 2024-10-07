#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhyisicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
	vertexCount = vertices->size();
	physicalDevice = newPhyisicalDevice;
	device = newDevice;
	createVertexBuffer(vertices);

}

int Mesh::getVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::destoryVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, deviceMemory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	VkDeviceSize bfSize = sizeof(Vertex)* vertices->size();
	createBuffer(physicalDevice, device, bfSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexBuffer, &deviceMemory);

	void* data;
	vkMapMemory(device, deviceMemory, 0, bfSize, 0, &data);
	memcpy(data, vertices->data(), (size_t)bfSize);
	vkUnmapMemory(device, deviceMemory);
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedType, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((allowedType & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & flags) == flags)
			
			return i;
	}
}
