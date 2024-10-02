#include "VulkanRender.h"

VulkanRender::VulkanRender() {
}



int VulkanRender::init(GLFWwindow* newWindow)
{	
	window = newWindow;
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		

	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	
	return 0;
}

void VulkanRender::cleanUp()
{			
	for (const auto& image : images) {
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
	
}

VulkanRender::~VulkanRender()
{
}

void VulkanRender::getPhysicalDevice()
{
	VkCommandBuffer commandBuffer;  // Invalid operation before logical device creation
	uint32_t deviceCount = 0;
	if(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)!= VK_SUCCESS)
		throw std::runtime_error("Failed during enum Phyiscal Devices");
	
	if (deviceCount == 0)
		throw std::runtime_error("Devices not support Vulkan");

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	for (const auto& device : physicalDevices) {
		if (checkDeviceSuitable(device)) {
			mainDevice.physicalDevice = device;
			break;
		}		
	}
	if (mainDevice.physicalDevice == nullptr) {
		throw std::runtime_error("No suitable physical Device");
	}
}

SwapChainDetails VulkanRender::getSwapChainDetails(VkPhysicalDevice device)
{	
	//PER device/surface
	SwapChainDetails swapChainDetails;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);
	uint32_t formatCount;
	uint32_t presentationCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	
	if (formatCount != 0) 
	{
		swapChainDetails.imageFormat.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.imageFormat.data());
	}
	
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount,nullptr);

	if (presentationCount != 0) 
	{	
		swapChainDetails.presentationsMode.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationsMode.data());
	}

	return swapChainDetails;
}

void VulkanRender::createInstance()
{	
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Application requires validation layer but not available");
	}
	
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan test";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.pApplicationInfo = &appInfo;
	
	uint32_t extensionCount;
	std::vector<const char*> instanceExtensions;
	const char** glfwExtensions;

	//instance extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	
	for (size_t k = 0; k < extensionCount; k++) {
		instanceExtensions.push_back(glfwExtensions[k]);
	}
	if (enableValidationLayers) {
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	if (!checkInstanceExtensionSupport(&instanceExtensions))
		throw std::runtime_error("Vk instance does not support required Extension");

	createInfo.enabledExtensionCount = extensionCount+1;
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());;
		createInfo.ppEnabledLayerNames = validationLayers.data();
		printf("%s", validationLayers[0]);
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	

	//create Instance
	VkResult res =  vkCreateInstance(&createInfo, nullptr, &instance);
	if (res != EXIT_SUCCESS) {
		throw std::runtime_error("Failed to Create Instance");
	}
}

void VulkanRender::createLogicalDevice()
{
	QueueFamilyIndices ind = getQueueFamilies(mainDevice.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos; 
	std::set<int> queuesIndex = { ind.graphicsFamily, ind.presentationFamily };

	std::set<int>::iterator it;
	for (it = queuesIndex.begin(); it != queuesIndex.end(); ++it) {
		//queues for logical Device
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = *it;
		queueInfo.queueCount = 1;
		float priority = 1.0f;
		queueInfo.pQueuePriorities = &priority;

		deviceQueueInfos.push_back(queueInfo);
	}

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = static_cast<uint32_t> (deviceQueueInfos.size());
	deviceInfo.pQueueCreateInfos = deviceQueueInfos.data();
	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
	
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceInfo.pEnabledFeatures = &deviceFeatures; //physical device features, device will use

	if(vkCreateDevice(mainDevice.physicalDevice, &deviceInfo, nullptr, &mainDevice.logicalDevice)!= VK_SUCCESS)
		throw std::runtime_error("failed to create Logical Device");
	
	vkGetDeviceQueue(mainDevice.logicalDevice, ind.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, ind.presentationFamily, 0, &presentationQueue);
}

void VulkanRender::createSurface()
{
	//create surface (create surface create info struct similar as glfw)
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed creating surface");
	}


}

void VulkanRender::createSwapChain()
{

	//CHoose best swapchainfeature
	SwapChainDetails swChainDetails = getSwapChainDetails(mainDevice.physicalDevice);
	
	VkSurfaceFormatKHR format =  chooseBestFormatSurface(swChainDetails.imageFormat);
	VkPresentModeKHR mode = chooseBestPresentMode(swChainDetails.presentationsMode);
	VkExtent2D extent = chooseSwapExtent(swChainDetails.surfaceCapabilities);

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.presentMode = mode;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	if (swChainDetails.surfaceCapabilities.maxImageCount > 0)
		createInfo.minImageCount = std::min(swChainDetails.surfaceCapabilities.minImageCount + 1, swChainDetails.surfaceCapabilities.maxImageCount); //+1 for tripple buffering
	else
		createInfo.minImageCount = swChainDetails.surfaceCapabilities.minImageCount + 1;
	createInfo.imageArrayLayers = 1; //number of layers per each array
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //normally in swapchain always color
	createInfo.preTransform = swChainDetails.surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //Handle blending images with more windows
	createInfo.clipped = VK_TRUE;

	//getQueueFam+
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	//if presentation and graphics queues are different, swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily) {
		uint32_t queueFamilyIndex[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	//if old one gets destoyed, this one repkaces it exactly the same
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mainDevice.logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed creating Swapchain");

	swapChainExtent2D = extent;
	swapChainFormat = format.format;

	uint32_t images_count;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &images_count, nullptr);

	if (images_count == 0)
		throw std::runtime_error("No images in SwapChain!");
	
	std::vector<VkImage> swImages(images_count);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &images_count, swImages.data());

	for (const auto& image : swImages) {
		SwapChainImage swImage = {};
		swImage.image = image;
		swImage.imageView = createImageView(image, swapChainFormat,VK_IMAGE_ASPECT_COLOR_BIT );

		images.push_back(swImage);

	}



}

bool VulkanRender::checkInstanceExtensionSupport(std::vector<const char*>* check) {
	uint32_t extensionCount = 0;
	//we don't know yet size of list so we h ave to first obtain count
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& checkExtension : *check) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp(checkExtension, extension.extensionName)==0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) {
			return false;
		}
			
	}
	return true;
}
bool VulkanRender::checkDeviceSuitable(VkPhysicalDevice device)
{	
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//queue families
	QueueFamilyIndices ind = getQueueFamilies(device);
	//swapchain extension
	bool deviceExtensionSupport = checkDeviceExtensionSupport(device);

	//see if device swapchain/surface has format/presentation modes.
	bool swapChainValid=false;
	if (deviceExtensionSupport) 
	{
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = swapChainDetails.imageFormat.size() > 0 && swapChainDetails.presentationsMode.size() > 0;
	}
		

	return ind.isValid() && deviceExtensionSupport && swapChainValid;
}

bool VulkanRender::checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); 

	for (const auto& validationLayer : validationLayers) {
		bool layerFound = false;
		for(const auto& availableLayer : availableLayers){
			if(strcmp(validationLayer, availableLayer.layerName)==0){
				layerFound = true;
				break;	
			}
		}
		if (!layerFound) {
			return false;
		}
	}

	return true;

}

bool VulkanRender::checkDeviceExtensionSupport(VkPhysicalDevice device)
{	
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	
	if (extensionCount == 0)
		return false;

	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionProperties.data());

	for (const auto& neededExtensions : deviceExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensionProperties)
		{
			if (strcmp(neededExtensions, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
			return false;

	}
		
	return true;
}

VkSurfaceFormatKHR VulkanRender::chooseBestFormatSurface(const std::vector<VkSurfaceFormatKHR>& formats)
{	
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}
	for (const auto& format : formats) {
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return formats[0];
}

VkPresentModeKHR VulkanRender::chooseBestPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRender::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{   
	//if extent is in numeric limits, extent may vary. If not, its already the size of the window.
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		//surface also defines max and min, so make sure no clamping value. so it complies with surface sizes.
		newExtent.width = std::max(std::min(capabilities.maxImageExtent.width, newExtent.width),capabilities.minImageExtent.width);
		//surface also defines max and min, so make sure no clamping value. so it complies with surface sizes.
		newExtent.height = std::max(std::min(capabilities.maxImageExtent.height, newExtent.height), capabilities.minImageExtent.height);

		return newExtent;

	}
}




QueueFamilyIndices VulkanRender::getQueueFamilies(VkPhysicalDevice device)
{	
	QueueFamilyIndices queueFamily;
	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queues (queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queues.data());
	int i = 0;
	for (const auto& queue : queues) {
		if (queue.queueCount > 0 && queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamily.graphicsFamily = i;
		}
		//check if queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		if (presentationSupport && queue.queueCount>0)
			queueFamily.presentationFamily = i;
		if (queueFamily.isValid())
			break;
		i++;
	}
	return queueFamily;

}


void VulkanRender::setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void VulkanRender::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}



 VkResult VulkanRender::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	 auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	 if (func != nullptr) {
		 return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	 }
	 else {
		 return VK_ERROR_EXTENSION_NOT_PRESENT;
	 }
 }

 VkImageView VulkanRender::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
 {
	 VkImageViewCreateInfo viewInfo = {};
	 viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	 viewInfo.image = image;
	 viewInfo.format = format;
	 viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	 viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	 viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	 viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	 viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	 viewInfo.subresourceRange.aspectMask = aspectFlags;
	 viewInfo.subresourceRange.baseMipLevel = 0;
	 viewInfo.subresourceRange.levelCount = 1; //number of mip map
	 viewInfo.subresourceRange.baseArrayLayer = 0;
	 viewInfo.subresourceRange.layerCount = 1; //array levels to view. textures or 3d images

	 VkImageView imageView;
	 if (vkCreateImageView(mainDevice.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		 throw std::runtime_error("Failed Image View Creation");
	 }

	 return imageView;
 }

 void VulkanRender::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	 auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	 if (func != nullptr) {
		 func(instance, debugMessenger, pAllocator);
	 }
 }




