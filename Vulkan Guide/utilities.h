#pragma once

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
// indices of locations of queue family in gpu
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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