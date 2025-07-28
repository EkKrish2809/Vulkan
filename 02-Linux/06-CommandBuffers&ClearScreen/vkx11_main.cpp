// x11 libraries: The project uses the X11 libraries for window management and input handling.
#include <X11/Xlib.h> 
#include <X11/Xutil.h> // XVisualInfo
#include <X11/XKBlib.h> // for KeyBoard

// vulkan related headers
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <memory.h> // for memset()
#include <iostream>
#include <vector>

// window size
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// window related variables
Display *display = NULL;
XVisualInfo *visualInfo = NULL;
Colormap colorMap;
Window window;
Bool fullscreen = False;
Bool b_CloseLoop = False;
Bool bActiveWindow = False;

// Log file creation 
FILE *VkLogFile = NULL;
const char* vkLogFileName = "VkApp_Log.txt";

// Vulkan related veriables
VkInstance instance;
VkPhysicalDevice g_PhysicalDevice;
VkDevice g_LogicalDevice;
VkSurfaceKHR surface;
VkSurfaceFormatKHR g_SwapChainSurfaceFormat;
VkSwapchainKHR g_SwapChain = VK_NULL_HANDLE;
std::vector<VkImage> swapchainImages;
VkCommandPool g_CommandPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> g_CommandBuffer;
uint32_t g_CommandBufferCount = 0; // number of command buffers should be equal to the number of swapchain images
VkQueue g_Queue = VK_NULL_HANDLE;
VkSemaphore g_RenderCompleteSem = VK_NULL_HANDLE;
VkSemaphore g_PresentCompleteSem = VK_NULL_HANDLE;

// for debugging
VkDebugUtilsMessengerEXT g_DebugUtilsMessenger = VK_NULL_HANDLE;

const char* GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERROR";
	default: return "UNKNOWN";
	}
}
const char* GetDebugType(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "GENERAL";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "VALIDATION";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "DEVICE_ADDRESS_BINDING";
	default: return "UNKNOWN";
	}
}

// debug callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
													VkDebugUtilsMessageTypeFlagsEXT type,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													void* pUserData)
{
	fprintf(VkLogFile, "Debug Callback : %s\n", pCallbackData->pMessage);
	fprintf(VkLogFile, "\tSeverity : %s\n", GetDebugSeverityStr(severity));
	fprintf(VkLogFile, "\tType : %s\n", GetDebugType(type));
	fprintf(VkLogFile, "\tObjects ");

	for (uint32_t i=0; i < pCallbackData->objectCount; i++)
	{
		// if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			fprintf(VkLogFile, "%llx \n", pCallbackData->pObjects[i].objectHandle);
			fprintf(VkLogFile, "\tObject %d : %s\n", i, pCallbackData->pObjects[i].pObjectName ? pCallbackData->pObjects[i].pObjectName : "No Name");
		}
	}

	return VK_FALSE; // return false to continue with the Vulkan operation
}

void createDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
						VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = nullptr
	};

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger)
	{
		std::cerr << "Failed to get address for vkCreateDebugUtilsMessengerEXT\n";
		return;
	}

	if (vkCreateDebugUtilsMessenger(instance, &messengerCreateInfo, nullptr, &g_DebugUtilsMessenger) != VK_SUCCESS)
	{
		std::cerr << "Failed to create debug utils messenger\n";
		return;
	}

	std::cout << "Debug callback created successfully\n";
}

void destroyDebugUtilsMessenger()
{
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugUtilsMessenger)
	{
		std::cerr << "Failed to get address for vkCreateDebugUtilsMessengerEXT\n";
		return;
	}

	vkDestroyDebugUtilsMessenger(instance, g_DebugUtilsMessenger, nullptr);
	std::cout << "Debug callback destroyed successfully\n";
}

// logger function
/* will open the log file in append mode, write the log message and close the log file to avoid loosing and looged messages due to unclosed log file pointer */
void logMessage(const char* message)
{
	
}

// =====================================================================================================================================
// Entry point
int main()
{
    // function declarations
    void toggleFullscreen(void);
	void queryInstanceLayersAndExtensions(void);
	void createSwapChain(void);
	void createCommandBuffers(void);
	void recordCommandBuffers(void);
	void initializeQueue(void);
	void update(void);
	void render(void);

	// local variables
	int numFBConfig;

    XSetWindowAttributes windowAttribute;
	int styleMask;
	Atom wm_delete_window_atom;
	XEvent event;
	KeySym keysym;
	int screenWidth;
	int screenHeight;
	char keys[26];

	// open the log file
	VkLogFile = fopen(vkLogFileName, "w");
	if (!VkLogFile)
	{
		std::cerr << "Failed to open log file: " << vkLogFileName << "\n";
		return -1;
	}

    // Open a connection to the X server
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        std::cerr << "Unable to open X display\n";
        return -1;
    }

    // Create a simple window
    // int screen = DefaultScreen(display);
    int screen = XDefaultScreen(display);
	
	// Use the default visual for the screen
	visualInfo = XGetVisualInfo(display, 0, NULL, &numFBConfig);
	if (!visualInfo) {
		std::cerr << "Unable to get visual info\n";
		XCloseDisplay(display);
		return -1;
	}
	printf( "visualId of bestVisualInfo is 0x%lu\n", visualInfo->visualid);


    // setting window attributes
    memset(&windowAttribute, 0, sizeof(XSetWindowAttributes));
	windowAttribute.border_pixel = 0;
	windowAttribute.background_pixel = XBlackPixel(display, screen);
	windowAttribute.background_pixmap = 0; // bkg picture vapraychay ka asa meaning hoto
	windowAttribute.colormap = XCreateColormap(display, 
						RootWindow(display, visualInfo->screen),
						visualInfo->visual,
						AllocNone);
	windowAttribute.event_mask = ExposureMask | KeyPressMask| StructureNotifyMask | FocusChangeMask; 

    // setting colormap
	colorMap = windowAttribute.colormap;

	// window style mask
	styleMask = CWBorderPixel | CWBackPixel | CWColormap | CWEventMask;

	//
	window = XCreateWindow(display, RootWindow(display, visualInfo->screen),
			100, // x
			100, // y
			WIN_WIDTH, // width
			WIN_HEIGHT, // height
			0, // border width
			visualInfo->depth,  // depth of window
			InputOutput,
			visualInfo->visual,
			styleMask,
			&windowAttribute); 
	if (!window)
	{
		printf("ERROR : XCreateWindow() Failed !!!\n");
		exit(1);
	}


    // for closing window ... Xwindow does not have close button by default, 
	// we ask for it to Window Manager(GNOME, KDE, etc)
	wm_delete_window_atom = XInternAtom(display, "WM_DELETE_WINDOW", True); // WM -> Window Manager
	XSetWMProtocols(display, window, &wm_delete_window_atom, 1);

    XMapWindow(display, window);

    // centering of Window
	screenWidth = XWidthOfScreen(XScreenOfDisplay(display, screen)); 
	screenHeight = XHeightOfScreen(XScreenOfDisplay(display, screen));
	XMoveWindow(display, window, screenWidth/2 - WIN_WIDTH/2, screenHeight/2 - WIN_HEIGHT/2);

    // Set the title of the window
    XStoreName(display, window, "Vulkan X11 Example");

    // =========================== Initialize Vulkan ===========================
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan X11 Example";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

	// xlib specific extension + debug extensions
	 std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME // for debugging
			// VK_EXT_DEBUG_REPORT_EXTENSION_NAME, // for debugging but deprecated
        };

	// enable validation layers if needed
	std::vector<const char*> validationLayers = {
		 "VK_LAYER_KHRONOS_validation" 
		};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
	createInfo.flags = 0;	// for future use
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance\n";
        return -1;
    }

	// create debug callback
	createDebugCallback();

	// find the physical device using vulkan instance
	uint32_t physicalDeviceCount = 0;
	if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to find Vulkan supported Physical Device\n";
		vkDestroyInstance(instance, nullptr);
		return -1;
	}
	else {
		// get the physical devices and print them
		std::vector<VkPhysicalDevice> physicalDeviceSupported(physicalDeviceCount);
		if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDeviceSupported.data()) != VK_SUCCESS)
		{
			std::cerr << "Failed to find Vulkan supported Physical Device\n";
			vkDestroyInstance(instance, nullptr);
			return -1;
		}

		// iterate through all the Physical devices found and Choose the suitable one
		for (const auto& dev : physicalDeviceSupported)
		{
			// Get the Vulkan supported Physical device properties & physical device features and select suitable Physical Device (GPU)
			VkPhysicalDeviceProperties deviceProperties = {};
			VkPhysicalDeviceFeatures deviceFeatures = {};
			
			vkGetPhysicalDeviceProperties(dev, &deviceProperties);
			vkGetPhysicalDeviceFeatures(dev, &deviceFeatures);

			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader)
			{
				// std::cout << "Device Name :: " << dev << "\n";
				std::cout << "\n####### Physical Device Properties ########\n";
				std::cout << "We will be using this Physical Device -> " << dev << "\n";
				std::cout << "API Version \t\t: " << VK_API_VERSION_VARIANT( deviceProperties.apiVersion) << "." 
													<< VK_API_VERSION_MAJOR(deviceProperties.apiVersion) << "." 
													<< VK_API_VERSION_MINOR( deviceProperties.apiVersion) << "." 
													<< VK_API_VERSION_PATCH( deviceProperties.apiVersion) << "\n";
				std::cout << "Device ID \t\t: " << deviceProperties.deviceID << "\n";
				std::cout << "Device Name \t\t: " << deviceProperties.deviceName << "\n";
				std::cout << "Device Type \t\t: " << deviceProperties.deviceType << "\n";
				std::cout << "Driver Version \t\t: " << VK_API_VERSION_VARIANT( deviceProperties.driverVersion) << "." 
													<< VK_API_VERSION_MAJOR(deviceProperties.driverVersion) << "." 
													<< VK_API_VERSION_MINOR( deviceProperties.driverVersion) << "." 
													<< VK_API_VERSION_PATCH( deviceProperties.driverVersion) << "\n";
				std::cout << "Vendor ID \t\t: " << deviceProperties.vendorID << "\n";
				g_PhysicalDevice = dev;
				break;
			}
		}


	}

	if (vkGetPhysicalDeviceXlibPresentationSupportKHR(g_PhysicalDevice, 0, display, visualInfo->visualid) != VK_TRUE)
	{
		std::cerr << "Physical Device does not support Xlib presentation\n";
		vkDestroyInstance(instance, nullptr);
		return -1;
	}
	else
	{
		std::cout << "Physical Device supports Xlib presentation\n";
	}

	// create vk surface for Xlib implementation
	VkXlibSurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.dpy = display;
	surfaceCreateInfo.window = window;

	// Create a Vulkan Surface
	if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
		std::cerr << "Failed to create Vulkan surface\n";
		vkDestroyInstance(instance, nullptr);
		return -1;
	}

	/*
	{
		// Get the Vulkan supported Physical device properties 
		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(g_PhysicalDevice, &deviceProperties);
		std::cout << "\n####### Physical Device Properties ########\n";
		std::cout << "API Version : " << deviceProperties.apiVersion << "\n";
		std::cout << "Device ID : " << deviceProperties.deviceID << "\n";
		std::cout << "Device Name : " << deviceProperties.deviceName << "\n";
		std::cout << "Device Type : " << deviceProperties.deviceType << "\n";
		std::cout << "Driver Version : " << deviceProperties.driverVersion << "\n";
		std::cout << "Vendor ID : " << deviceProperties.vendorID << "\n";
		std::cout << "Pipeline Cache UUID [IMP in Pipeline Caching] : " << deviceProperties.pipelineCacheUUID << "\n";
		std::cout << "Buffer Image Granularity LIMIT : " << deviceProperties.limits.bufferImageGranularity << "\n"; // has so many properties in limits << EXPLORE >>
		std::cout << "Residency Std 3D Block Shape SPARSE Prop : " << deviceProperties.sparseProperties.residencyStandard3DBlockShape << "\n"; // << EXPLORE >> Properties related to sparse Texture

		// Get the Vulkan Supported Physical Device FEATURES
		VkPhysicalDeviceFeatures deviceFeatures = {};
		vkGetPhysicalDeviceFeatures(g_PhysicalDevice, &deviceFeatures);
		// std::cout << "\n####### Physical Device Properties ########\n";
		// std::cout << deviceFeatures.alphaToOne << "\n";
	}
	*/

	// get Physical Device Memory Properties
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {0};
	vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &deviceMemoryProperties);
	// std::cout << "\n######### Physical Device Memory Properties #########\n";
	// std::cout << deviceMemoryProperties.memoryHeapCount << "\n";
	// std::cout << deviceMemoryProperties.memoryHeaps->size << "\n";
	// std::cout << deviceMemoryProperties.memoryTypeCount << "\n";
	// std::cout << deviceMemoryProperties.memoryTypes->heapIndex << "\n";

	// Query the Device for its queue families
	uint32_t queueFamilyPropertyCount = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties = {}; 
	vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queueFamilyPropertyCount, nullptr);
	
	queueFamilyProperties.resize(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
	// std::cout << "\n######### Physical Device Queue Family Properties #########\n";
	// for (const auto& qp : queueFamilyProperties)
	// {
	// 	std::cout << qp.minImageTransferGranularity.width << "," << qp.minImageTransferGranularity.height << "," << qp.minImageTransferGranularity.depth << "\n";
	// 	std::cout << qp.queueCount << "\n";
	// 	std::cout << qp.queueFlags << "\n";
	// 	std::cout << qp.timestampValidBits << "\n";
	// }

	// ======= Creating logical device ==========
	VkPhysicalDeviceFeatures supportedFeatures = {};
	VkPhysicalDeviceFeatures requiredFeatures = {};
	vkGetPhysicalDeviceFeatures(g_PhysicalDevice, &supportedFeatures);

	// setting the required features
	requiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;
	requiredFeatures.tessellationShader = VK_TRUE;
	requiredFeatures.geometryShader = VK_TRUE;

	float qPriorities[] = {1.0f}; // highest priority
	const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
		.sType 				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,		// sType
		.pNext 				= nullptr,											// pNext
		.flags 				= 0,												// flags
		.queueFamilyIndex 	= 0,												// queueFamilyIndex // hardcoded to 0 for now, but need to find the suitable queue family index from QueueFamilyProperties
		.queueCount 		= 1,												// queueCount
		.pQueuePriorities 	= qPriorities										// pQueuePriorities
	};

	// extensions 
	std::vector<const char*> devExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, // for geometry shader
	};

	VkDeviceCreateInfo logicalDeviceCreateInfo = {};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = nullptr;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = 1;
	logicalDeviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	logicalDeviceCreateInfo.enabledExtensionCount = (uint32_t)devExtensions.size();
	logicalDeviceCreateInfo.ppEnabledExtensionNames = devExtensions.data();
	logicalDeviceCreateInfo.pEnabledFeatures = &requiredFeatures;

	if (vkCreateDevice(g_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &g_LogicalDevice) != VK_SUCCESS)
	{
		std::cerr << "Failed to create Logical Device\n";
		return -1;
	}

	// Query the instance-level layers and extensions
	queryInstanceLayersAndExtensions();

	// create the swapchain
	createSwapChain();

	// create command buffers
	createCommandBuffers();

	// record command buffers
	recordCommandBuffers();

	// initialize the queue for rendering
	initializeQueue();

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    // Main loop
    while (b_CloseLoop == False) 
	{
        XEvent event;
		while (XPending(display)) // XPending means Peak Message
		{
			XNextEvent(display, &event);
			switch (event.type)
			{
			case MapNotify:
				break;

			case FocusIn:
				bActiveWindow = True;
				break;

			case FocusOut:
				bActiveWindow = True;
				break;

			case KeyPress:
				keysym = XkbKeycodeToKeysym(display, event.xkey.keycode, 0, 0);
				switch (keysym)
				{
				case XK_Escape:
					b_CloseLoop = True;
					break;

				case XK_Left:
					break;
				case XK_Right:
					break;
				case XK_Up:
					break;
				case XK_Down:
					break;
				case XK_KP_Add:
					break;
				case XK_KP_Subtract:
					break;
				}

				XLookupString(&event.xkey, keys, sizeof(keys), NULL, NULL);
				
				switch (keys[0])
				{
				case 'F':
				case 'f':
					if (fullscreen == False)
					{
						toggleFullscreen();
						fullscreen = True;
					}
					else
					{
						toggleFullscreen();
						fullscreen = False;
					}
					break;
				case 'c':
					
					break;
				}
				break;
			case MotionNotify:
				break;
			case ButtonPress:
				break;
			case ButtonRelease:
				break;

			case ConfigureNotify:
				// winWidth = event.xconfigure.width;
				// winHeight = event.xconfigure.height;
				// resize(winWidth, winHeight); // LOWARD & HIWORD
				break;

			case ClientMessage:
				b_CloseLoop = True;
				break;
			}
		
		}

		if (bActiveWindow == True)
		{
			update();
			render();
		}
    }

    // Cleanup
	if (g_Queue != VK_NULL_HANDLE)
	{
		vkQueueWaitIdle(g_Queue);
		g_Queue = VK_NULL_HANDLE;
	}

	if (g_RenderCompleteSem != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(g_LogicalDevice, g_RenderCompleteSem, nullptr);
		g_RenderCompleteSem = VK_NULL_HANDLE;
	}
	if (g_PresentCompleteSem != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(g_LogicalDevice, g_PresentCompleteSem, nullptr);
		g_PresentCompleteSem = VK_NULL_HANDLE;
	}

	if (g_CommandBuffer.size() > 0)
	{
		for (auto& cmdBuffer : g_CommandBuffer)
		{
			vkFreeCommandBuffers(g_LogicalDevice, g_CommandPool, 1, &cmdBuffer);
		}
		g_CommandBuffer.clear();
	}

	if (g_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(g_LogicalDevice, g_CommandPool, nullptr);
	}

	if (g_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(g_LogicalDevice, g_SwapChain, nullptr);
		g_SwapChain = VK_NULL_HANDLE;
	}

	if (vkDeviceWaitIdle(g_LogicalDevice) == VK_SUCCESS)
		vkDestroyDevice(g_LogicalDevice, nullptr);
		
	vkDestroySurfaceKHR(instance, surface, nullptr); 
	destroyDebugUtilsMessenger();
	
	if (window)
		XDestroyWindow(display, window);
	
    XFreeColormap(display, colorMap);
	
	if (display)
		XCloseDisplay(display);
	
    vkDestroyInstance(instance, nullptr);

	if (VkLogFile)
	{
		fclose(VkLogFile);
		VkLogFile = NULL;
	}
    return 0;
}

void toggleFullscreen(void)
{
	// local variables declarations
	Atom wm_current_state_atom;
	Atom wm_fullscreen_state_atom;
	XEvent event;

	// code
	wm_current_state_atom = XInternAtom(display, "_NET_WM_STATE", False);
	wm_fullscreen_state_atom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

	memset(&event, 0, sizeof(XEvent));
	event.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = wm_current_state_atom;
	event.xclient.format = 32;
	event.xclient.data.l[0] = fullscreen ? 0 : 1;
	event.xclient.data.l[1] = wm_fullscreen_state_atom;

	// like sent and post messages in Windows OS
	XSendEvent(display,
			RootWindow(display, visualInfo->screen),
			False, // if this msg is for this window or for its child window
			SubstructureNotifyMask,
			&event);
}


// querying the instance level / device level layers and extensions
void queryInstanceLayersAndExtensions()
{
	// instance level layers
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount != 0)
	{
		std::cout << "\n####### " << layerCount << " instance layers supported ########\n";
		std::vector<VkLayerProperties> layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
		for (const auto& layer : layers) {
			std::cout << "Layer: " << layer.layerName  << " : " << layer.description << " : " << layer.specVersion << "\n";
		}
	}

	// device level layers
	/* TODO < Explore Device Level layers and Extensions > */


	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	if (extensionCount != 0)
	{
		std::cout << "\n####### " << extensionCount << " instance extensions supported ########\n";
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		for (const auto& ext : extensions) {
			std::cout << "Extension: " << ext.extensionName << " : " << ext.specVersion << "\n";
		}
	}
}

// create swapchain
void createSwapChain()
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
	{
		std::cerr << "Failed to get physical device surface capabilities\n";
		return;
	}

	// choose num of images in swapchain
	uint32_t requestedNumImages = surfaceCapabilities.minImageCount + 1; // at least minImageCount+1
	
	uint32_t finalNumImages = 0;
	if ((surfaceCapabilities.maxImageCount > 0) && (requestedNumImages > surfaceCapabilities.maxImageCount))
	{
		finalNumImages = surfaceCapabilities.maxImageCount;
	}
	else
	{
		finalNumImages = requestedNumImages;
	}
	g_CommandBufferCount = finalNumImages;

	// find the number of present modes supported by the surface
	uint32_t numPresentModes = 0;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, surface, &numPresentModes, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to get physical device surface present mode count\n";
	}
	
	// fetch the present modes 
	std::vector<VkPresentModeKHR> presentModes(numPresentModes);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, surface, &numPresentModes, presentModes.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to get physical device surface present modes\n";
	}
	else 
	{
		std::cout << "\n######### Physical Device Surface Present Modes #########\n";
		for (const auto& mode : presentModes)
		{
			switch (mode)
			{
			case VK_PRESENT_MODE_IMMEDIATE_KHR:
				std::cout << "VK_PRESENT_MODE_IMMEDIATE_KHR\n";
				break;
			case VK_PRESENT_MODE_MAILBOX_KHR:
				std::cout << "VK_PRESENT_MODE_MAILBOX_KHR\n";
				break;
			case VK_PRESENT_MODE_FIFO_KHR:
				std::cout << "VK_PRESENT_MODE_FIFO_KHR\n";
				break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
				std::cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR\n";
				break;
			default:
				std::cout << "Unknown Present Mode\n";
				break;
			}
		}
	}

	// choose surface format and color space
	// 1. get the number of surface formats supported by the physical device
	uint32_t numSurfaceFormats = 0;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, surface, &numSurfaceFormats, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to get physical device surface format count\n";
		return;
	}

	// 2. fetch the surface formats
	std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, surface, &numSurfaceFormats, surfaceFormats.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to get physical device surface format\n";
		return;
	}
	else
	{
		std::cout << "\n######### Physical Device Surface Formats #########\n";
		for (int i=0; i < numSurfaceFormats; i++)
		{
			if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				g_SwapChainSurfaceFormat = surfaceFormats[i]; // choose this format
				std::cout << "Format: " << surfaceFormats[i].format << ", Color Space: " << surfaceFormats[i].colorSpace << "\n";
			}
		}
		
	}

	// fill the swapchain create infor struct
	VkSwapchainCreateInfoKHR swapchainCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = surface,
		.minImageCount = finalNumImages,
		.imageFormat = g_SwapChainSurfaceFormat.format,
		.imageColorSpace = g_SwapChainSurfaceFormat.colorSpace,
		.imageExtent = surfaceCapabilities.currentExtent, // {width, height} of the swapchain images
		.imageArrayLayers = 1, // number of layers in the swapchain images
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // exclusive mode for now,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = nullptr,  // queue family index address is passed here
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR, // can be changed to VK_PRESENT_MODE_FIFO_KHR 
		.clipped = VK_TRUE, // clipped means the swapchain images will not be visible to the user
	};

	// create the swapchain
	if (vkCreateSwapchainKHR(g_LogicalDevice, &swapchainCreateInfo, nullptr, &g_SwapChain) != VK_SUCCESS)
	{
		std::cerr << "Failed to create swapchain\n";
		return;
	}
	else
	{
		std::cout << "Swapchain created successfully\n";
	}

	// create the image views for the swapchain images
	uint32_t swapchainImagesCount = 0;
	if (vkGetSwapchainImagesKHR(g_LogicalDevice, g_SwapChain, &swapchainImagesCount, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to get swapchain images count\n";
		return;
	}

	swapchainImages.resize(swapchainImagesCount);
	if (vkGetSwapchainImagesKHR(g_LogicalDevice, g_SwapChain, &swapchainImagesCount, swapchainImages.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to get swapchain images\n";
		return;
	}
}

// create the command pool and command buffers
void createCommandBuffers()
{
	// create command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = 0
	};

	if (vkCreateCommandPool(g_LogicalDevice, &commandPoolCreateInfo, nullptr, &g_CommandPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create command pool\n";
		return;
	}

	// create command buffers
	VkCommandBufferAllocateInfo commandBufferAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = g_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // primary command buffer
		.commandBufferCount = g_CommandBufferCount // command buffer count == swapchain image count
	};

	g_CommandBuffer.resize(g_CommandBufferCount);
	if (vkAllocateCommandBuffers(g_LogicalDevice, &commandBufferAllocateInfo, g_CommandBuffer.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate command buffers\n";
		return;
	}


}

// record command buffers
void recordCommandBuffers()
{
	VkClearColorValue clearColor = {1.0f, 0.0f, 0.0f, 1.0f}; // red color

	VkImageSubresourceRange subresourceRange = 
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	for (uint32_t i=0; i < g_CommandBuffer.size(); i++)
	{
		// create image barrier to transition
		VkImageMemoryBarrier presentToClearBarrier = 
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = swapchainImages.at(i),
			.subresourceRange = subresourceRange
		};

		VkImageMemoryBarrier clearToPresentBarrier = 
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = swapchainImages.at(i),
			.subresourceRange = subresourceRange
		};

		VkCommandBufferBeginInfo commandBufferBeginInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			.pInheritanceInfo = nullptr // for secondary command buffers
		};

		if (vkBeginCommandBuffer(g_CommandBuffer.at(i), &commandBufferBeginInfo) != VK_SUCCESS)
		{
			std::cerr << "Failed to begin command buffer\n";
			return;
		}

		// transition the image layout from present to clear
		vkCmdPipelineBarrier(g_CommandBuffer.at(i), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
							0,  // dependency flags
							0, NULL, // memory barriers
							0, NULL, // buffer memory barriers
							1, &presentToClearBarrier); // image memory barriers

		// clear the color attachment
		vkCmdClearColorImage(g_CommandBuffer.at(i), swapchainImages.at(i), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &subresourceRange);

		// transition the image layout from clear to present
		vkCmdPipelineBarrier(g_CommandBuffer.at(i), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
							0,  // dependency flags
							0, NULL, // memory barriers
							0, NULL, // buffer memory barriers
							1, &clearToPresentBarrier); // image memory barriers

		// end the command buffer
		if (vkEndCommandBuffer(g_CommandBuffer.at(i)) != VK_SUCCESS)
		{
			std::cerr << "Failed to end command buffer\n";
			return;
		}
	}
}

// initialize the queue for rendering
void initializeQueue()
{
	vkGetDeviceQueue(g_LogicalDevice, 0, 0, &g_Queue);

	// create the semaphores for synchronization
	VkSemaphoreCreateInfo semaphoreCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0 // no flags for now
	};

	// create render complete semaphore
	if (vkCreateSemaphore(g_LogicalDevice, &semaphoreCreateInfo, nullptr, &g_RenderCompleteSem) != VK_SUCCESS)
	{
		std::cerr << "Failed to create render complete semaphore\n";
		return;
	}

	// create presentation complete semaphore
	if (vkCreateSemaphore(g_LogicalDevice, &semaphoreCreateInfo, nullptr, &g_PresentCompleteSem) != VK_SUCCESS)
	{
		std::cerr << "Failed to create render complete semaphore\n";
		return;
	}
}

// create vulkan buffers
VkBuffer createVulkanBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE)
{
	static const VkBufferCreateInfo bufferCreateInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 									// sType
		nullptr,                               									// pNext
		0,                                     									// flags
		1024 * 1024,                                     						// size can be a parameter
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,    // usage can be decided based on usecase ((as a parameter))
		sharingMode,             												// sharingMode
		0,                                     									// queueFamilyIndexCount
		nullptr                                									// pQueueFamilyIndices
	};

	VkBuffer buffer = VK_NULL_HANDLE;
	// create the buffer
	if (vkCreateBuffer(g_LogicalDevice, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		std::cerr << "Failed to create Vulkan buffer\n";
		return VK_NULL_HANDLE;
	}

	return buffer;
}

bool initialize()
{
    


    return true; // Placeholder for initialization logic
}

void render()
{
	// acquire the next image from the swapchain
	uint32_t imageIndex = -1;
	if (vkAcquireNextImageKHR(g_LogicalDevice, g_SwapChain, UINT64_MAX, g_PresentCompleteSem, nullptr, &imageIndex) != VK_SUCCESS)
	{
		std::cerr << "Failed to acquire next image from swapchain\n";
		return;
	}

	// submit the command buffer of above acquired image index asynchronously to the queue
	VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &g_PresentCompleteSem,
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &g_CommandBuffer.at(imageIndex),
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &g_RenderCompleteSem
	};

	if (vkQueueSubmit(g_Queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		std::cerr << "Failed to submit command buffer to queue\n";
		return;
	}

	// present the current image to the swapchain
	VkPresentInfoKHR presentInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &g_RenderCompleteSem,
		.swapchainCount = 1,
		.pSwapchains = &g_SwapChain,
		.pImageIndices = &imageIndex
	};

	if (vkQueuePresentKHR(g_Queue, &presentInfo) != VK_SUCCESS)
	{
		std::cerr << "Failed to present image to swapchain\n";
		return;
	}

	vkQueueWaitIdle(g_Queue); // wait for the queue to finish rendering

}

void update()
{

}