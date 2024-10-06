#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Mesh.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include "utilities.h"
#include <set>
#include <algorithm>
#include <array>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t  MAX_FRAME = 2;

class VulkanRender
{
public :

	VulkanRender();

	int init(GLFWwindow* newWindow);
	void draw();
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
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapChainImage> images;
	std::vector<VkFramebuffer> framebuffer;
	std::vector<VkCommandBuffer> commandBuffers; 

	Mesh mesh;

	int currentFrame = 0;

	//utility
	VkFormat swapChainFormat;
	VkExtent2D swapChainExtent2D;

	//synch
	std::vector<VkSemaphore> imagesAvailable;
	std::vector<VkSemaphore> rendersFinished;
	std::vector<VkFence> drawFence;
	
	//Pipeline
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;

	//Pools
	VkCommandPool graphCommandPool;

	//get functions
	void getPhysicalDevice();
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

	//create Functions
	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffer();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();

	//record 
	void recordCommand();

	//support
	bool checkInstanceExtensionSupport(std::vector<const char*>* extensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device); //swapchain compatibility is checked on physical device level


	//choose functions
	VkSurfaceFormatKHR chooseBestFormatSurface(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);


	//create support functions
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	//own debugger
	VkDebugUtilsMessengerEXT debugMessenger;
	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;  // Return false to let Vulkan continue
	}

	// Function to create the debug messenger
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	// Function to destroy the debug messenger
	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);
};

