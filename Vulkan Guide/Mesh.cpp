#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhyisicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices, VkCommandPool transferPool, VkQueue transferQueue)
{
	vertexCount = vertices->size();
	physicalDevice = newPhyisicalDevice;
	device = newDevice;
	createVertexBuffer(vertices,transferPool, transferQueue);
	
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

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices, VkCommandPool commandPool, VkQueue queue)
{
	VkDeviceSize bfSize = sizeof(Vertex)* vertices->size();

	//temporal stagingb buffer;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	

	createBuffer(physicalDevice, device, bfSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bfSize, 0, &data);
	memcpy(data, vertices->data(), (size_t)bfSize);
	vkUnmapMemory(device, stagingBufferMemory);

	//create Buffer as recipient of transfer

	createBuffer(physicalDevice, device, bfSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &deviceMemory);

	copyBuffer(device, bfSize, stagingBuffer, vertexBuffer, commandPool, queue);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
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
