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
		

	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	
	return 0;
}

void VulkanRender::cleanUp()
{	
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
	bool deviceExtensionSupport = checkDeviceExtensionSupport(device);
	return ind.isValid() && deviceExtensionSupport;
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

 void VulkanRender::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	 auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	 if (func != nullptr) {
		 func(instance, debugMessenger, pAllocator);
	 }
 }




