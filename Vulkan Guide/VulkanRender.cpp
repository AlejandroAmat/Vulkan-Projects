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

		std::vector<Vertex> meshVertices = {
			{{0.4, -0.4, 0.0},{1.0, 0.0, 0.0}},  // First vertex
			{{0.4, 0.4, 0.0}, {0.0, 1.0, 0.0}},   // Second vertex
			{{-0.4, 0.4, 0.0},{0.0, 0.0, 1.0}},
			{{-0.4, 0.4, 0.0},{0.0, 0.0, 1.0}},  // First vertex
			{{-0.4, -0.4, 0.0},{0.0, 1.0, 0.0}},   // Second vertex
			{{0.4, -0.4, 0.0}, {1.0, 0.0, 0.0}}
			// Third vertex
		};
		mesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, &meshVertices);
		createSwapChain();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffer();
		createCommandPool();
		createCommandBuffers();
		recordCommand();
		createSynchronization();

	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	
	return 0;
}

void VulkanRender::draw()
{

	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFence[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(mainDevice.logicalDevice, 1, &drawFence[currentFrame]);
	//.1 get next available imaghe to draw. use semaphores
	uint32_t ind;
	vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imagesAvailable[currentFrame], VK_NULL_HANDLE, &ind);
	
	//.2 Submit command buffer to queue for execution. wait for imaeg to be signaled.
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imagesAvailable[currentFrame];
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[ind];
	submitInfo.pSignalSemaphores = &rendersFinished[currentFrame];
	submitInfo.signalSemaphoreCount = 1;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFence[currentFrame]))
		throw std::runtime_error("Fail to submit Queue");
	//.3 present image to screen when signaled (finish rendered)

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &rendersFinished[currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &ind;

	if (vkQueuePresentKHR(presentationQueue, &presentInfo))
		throw std::runtime_error("Fail to create Image");

	currentFrame = (currentFrame + 1)%MAX_FRAME;
}

void VulkanRender::cleanUp()
{	
	vkDeviceWaitIdle(mainDevice.logicalDevice);
	mesh.destoryVertexBuffer();
	for (size_t i = 0; i < MAX_FRAME; i++)
	{
		vkDestroyFence(mainDevice.logicalDevice, drawFence[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, rendersFinished[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, imagesAvailable[i], nullptr);
	}
	
	vkDestroyCommandPool(mainDevice.logicalDevice, graphCommandPool, nullptr);
	for (const auto& fb : framebuffer)
	{
		vkDestroyFramebuffer(mainDevice.logicalDevice, fb, nullptr);
	}
	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass,  nullptr);
	for (const auto& image : images) 
	{
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
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndex;

	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
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

void VulkanRender::createRenderPass()
{
	VkAttachmentDescription colorAttatchment = {};
	colorAttatchment.format = swapChainFormat;
	colorAttatchment.samples = VK_SAMPLE_COUNT_1_BIT;  //multsampling
	colorAttatchment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //what attatchment before rend (clear 0 memory)
	colorAttatchment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttatchment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttatchment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

	//framebuffer will stored as image. but images can have different data loyouts.  Render view, source view...

	colorAttatchment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttatchment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Attatchment reference for subpass color att. 
	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0; //order in list
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	

	//need to determine when layout transitions occur--> subpass dependencies.
	std::array<VkSubpassDependency, 2> subpassDependency;
	subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	//has to happen before than
	subpassDependency[0].dstSubpass = 0;
	subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency[0].dependencyFlags = 0;

	subpassDependency[1].srcSubpass = 0;
	subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//has to happen before than
	subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependency[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttatchment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependency.size());
	renderPassInfo.pDependencies = subpassDependency.data();

	if (vkCreateRenderPass(mainDevice.logicalDevice, &renderPassInfo, nullptr, &renderPass)!=VK_SUCCESS)
		throw std::runtime_error("Failed creating render Pass");
}

void VulkanRender::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("Shaders/vert.spv");
	auto fragShaderCode = readFile("Shaders/frag.spv");

	VkShaderModule vertexShader = createShaderModule(vertShaderCode);
	VkShaderModule fragmentShader = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderCreate = {};
	vertexShaderCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreate.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreate.module = vertexShader;
	vertexShaderCreate.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreate = {};
	fragmentShaderCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreate.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreate.module = fragmentShader;
	fragmentShaderCreate.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreate, fragmentShaderCreate };

	//Pipeline
	//VERTEX INPUT 
	VkVertexInputBindingDescription bindingDescr = {};
	bindingDescr.binding = 0;
	bindingDescr.stride = sizeof(Vertex);
	bindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;   //how to move between with datra ag¡fter each vertex;

	std::array<VkVertexInputAttributeDescription,2> attr;
	attr[0].binding = 0;
	attr[0].location = 0;
	attr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr[0].offset = offsetof(Vertex, pos);

	attr[1].binding = 0;
	attr[1].location = 1;
	attr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attr[1].offset = offsetof(Vertex, col);

	VkPipelineVertexInputStateCreateInfo vertexIputCreateInfo = {};
	vertexIputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexIputCreateInfo.pVertexBindingDescriptions = &bindingDescr;  //data spacing, values stride
	vertexIputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexIputCreateInfo.pVertexAttributeDescriptions = attr.data();
	vertexIputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());

	// --INPUT ASSEMBLY--
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE; //overriding strip false

	//--viewport & scissor--
	VkViewport viewPort = {};
	viewPort.x = 0.0f;
	viewPort.y = 0.0f;
	viewPort.width = (float)swapChainExtent2D.width;
	viewPort.height = (float)swapChainExtent2D.height;
	viewPort.maxDepth = 1.0f;
	viewPort.minDepth = 0.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = swapChainExtent2D;

	VkPipelineViewportStateCreateInfo viewPortInfo = {};
	viewPortInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewPortInfo.pScissors = &scissor;
	viewPortInfo.pViewports = &viewPort;
	viewPortInfo.scissorCount = 1;
	viewPortInfo.viewportCount = 1;

	/*//Dynamic states// will not do in course but to understand
	std::vector<VkDynamicState> dynamicStatesEnables;
	dynamicStatesEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStatesEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);   //vkcmdSetViewport//scrissor commandBuffer, 0 (ind), 1 (size), &newViweport/scissor);
	//we have to desroty current swapchain and recreate it for new sizes
	VkPipelineDynamicStateCreateInfo dynStateInfo = {};
	dynStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStatesEnables.size());
	dynStateInfo.pDynamicStates = dynamicStatesEnables.data();
	*/
	//--rasterizer--
	
	VkPipelineRasterizationStateCreateInfo rasterInfo = {};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE; //if not want to send everything to frambuffer-- if not drawing /store intermediate val.
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL; //other needs gpuy feature
	rasterInfo.lineWidth = 1.0f;
	//rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;  //for shadow computations. for stopping shadow acne.

	//--multisampling--//
	//resolve attatchment
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  //samples per fragment

	//--Blending--
	//blend attatchemtnstate
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorState.blendEnable = VK_TRUE;
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;
	//to keep  alpha for some reason
	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendInfo = {};
	blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendInfo.logicOpEnable = VK_FALSE;  //alternative to calculations
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &colorState;
	//blendInfo.blendConstants if we want constant blendings

	//--Pipeline Layout (TODO:: Desccroiptor Set layouts//
	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;
	
	if (vkCreatePipelineLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed creating Pipeline Layout");

	//todo: depth stencil testing. No depth stuff
	//Destroy Modules

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexIputCreateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineInfo.pViewportState = &viewPortInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.pRasterizationState = &rasterInfo;
	pipelineInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineInfo.pColorBlendState = &blendInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // connect it to an existing pipeline
	pipelineInfo.basePipelineIndex = -1; //create more pipelines.
	//we can do cache pipelining
	if(vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)!=VK_SUCCESS)
		throw std::runtime_error("Fails creating Pipeline");

	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShader, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShader, nullptr);

	



}

void VulkanRender::createFramebuffer()
{
	framebuffer.resize(images.size());

	for (size_t i = 0; i < framebuffer.size(); i++)
	{
		std::array<VkImageView, 1> attatchments = {
			images[i].imageView
		};
		VkFramebufferCreateInfo fBufferCreate = {};
		fBufferCreate.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fBufferCreate.renderPass = renderPass;
		fBufferCreate.attachmentCount = static_cast<uint32_t>(attatchments.size());
		fBufferCreate.width = swapChainExtent2D.width;
		fBufferCreate.height = swapChainExtent2D.height;
		fBufferCreate.pAttachments = attatchments.data();
		fBufferCreate.layers = 1;

		if (vkCreateFramebuffer(mainDevice.logicalDevice, &fBufferCreate, nullptr, &framebuffer[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Frame Buffer");
	}
}

void VulkanRender::createCommandPool()
{
	QueueFamilyIndices ind = getQueueFamilies(mainDevice.physicalDevice);
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = ind.graphicsFamily;

	if(vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphCommandPool)!=VK_SUCCESS)
		throw std::runtime_error("Fail to create Command Pool");
}

void VulkanRender::createCommandBuffers()
{
	commandBuffers.resize(framebuffer.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = graphCommandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //executed by queue--secondary-> by other buffers

	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Fail to allocate buffers");

}

void VulkanRender::createSynchronization()
{	
	imagesAvailable.resize(MAX_FRAME);
	rendersFinished.resize(MAX_FRAME);
	drawFence.resize(MAX_FRAME);
	//semphore
	VkSemaphoreCreateInfo smphInfo = {};
	smphInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME; i++) {

		if (vkCreateSemaphore(mainDevice.logicalDevice, &smphInfo, nullptr, &imagesAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mainDevice.logicalDevice, &smphInfo, nullptr, &rendersFinished[i]) != VK_SUCCESS ||
			vkCreateFence(mainDevice.logicalDevice, &fenceInfo, nullptr, &drawFence[i]))
			throw std::runtime_error("Failed creating Semaphores and/or Fence");
	}
	
}

void VulkanRender::recordCommand()
{
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; //can be resubmitted to queue. just for now. NOT NEEDED

	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass = renderPass;
	rpInfo.renderArea.offset = { 0,0 };
	rpInfo.renderArea.extent = swapChainExtent2D;
	VkClearValue clearValue[] = { 0.6f, 0.65f, 0.4, 1.0f };
	rpInfo.pClearValues = clearValue;
	rpInfo.clearValueCount = 1;

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		rpInfo.framebuffer = framebuffer[i];

		if (vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo) != VK_SUCCESS)
			throw std::runtime_error("Fail to record Command Buffer");
		
			vkCmdBeginRenderPass(commandBuffers[i], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
				VkBuffer vertexBuffers[] = { mesh.getVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
				vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(mesh.getVertexCount()), 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Fail to stop recording Command Buffer");
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

VkShaderModule VulkanRender::createShaderModule(const std::vector<char>& code)
 {
	 VkShaderModuleCreateInfo shaderCreateInfo = {};
	 shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	 shaderCreateInfo.codeSize = code.size();
	 shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	 VkShaderModule shaderModule;
	 if (vkCreateShaderModule(mainDevice.logicalDevice, &shaderCreateInfo, nullptr, &shaderModule)!= VK_SUCCESS)
		 throw std::runtime_error("Clansy Error: Fail Creating Shader Module");

	 return shaderModule;

 }

void VulkanRender::setupDebugMessenger() 
{
	 if (!enableValidationLayers) return;

	 VkDebugUtilsMessengerCreateInfoEXT createInfo;
	 populateDebugMessengerCreateInfo(createInfo);

	 if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	 {
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



