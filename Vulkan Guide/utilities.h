#pragma once

#include <fstream>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
// indices of locations of queue family in gpu
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
};

struct QueueFamilyIndices {
	int graphicsFamily = -1; //location
	int presentationFamily = -1;
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily>=0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities; //surface properties
	std::vector<VkSurfaceFormatKHR> imageFormat;  //RGB, HSV...
	std::vector<VkPresentModeKHR> presentationsMode;  //presentationMode
};

struct SwapChainImage {
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("Failed opening file!");
	}
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);
	file.seekg(0);

	file.read(fileBuffer.data(), fileSize);

	file.close();

	return fileBuffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedType, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((allowedType & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & flags) == flags)

			return i;
	}
}

static void createBuffer(VkPhysicalDevice phisicalDevice, VkDevice device, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memFlags, VkBuffer* buffer,
	VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = deviceSize;
	bufferInfo.usage = bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, buffer) != VK_SUCCESS)
		throw std::runtime_error("Fail creating Buffer");

	VkMemoryRequirements memReq = {};
	vkGetBufferMemoryRequirements(device, *buffer, &memReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memReq.size;
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex( phisicalDevice ,memReq.memoryTypeBits, memFlags);

	if (vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate VB memory");

	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static void copyBuffer(VkDevice device, VkDeviceSize deviceSize, VkBuffer srcBuffer, VkBuffer dstBuffer, VkCommandPool transferPool, VkQueue transferQueue) 
{

	//create commandBuffer
	VkCommandBuffer transfBuffer;
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = transferPool;
	allocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(device, &allocateInfo, &transfBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkBufferCopy copy = {};
	copy.dstOffset = 0;
	copy.srcOffset = 0;
	copy.size = deviceSize;

	vkBeginCommandBuffer(transfBuffer, &beginInfo);
		vkCmdCopyBuffer(transfBuffer, srcBuffer, dstBuffer,1, &copy);
	vkEndCommandBuffer(transfBuffer);

	//queue submission

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transfBuffer;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, transferPool, 1, &transfBuffer);

}