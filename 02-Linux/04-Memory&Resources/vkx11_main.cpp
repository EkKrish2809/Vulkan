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


// Vulkan related veriables
VkPhysicalDevice g_PhysicalDevice;
VkDevice g_LogicalDevice;

int main()
{
    // function declarations
    void toggleFullscreen(void);
	void queryInstanceLayersAndExtensions(void);

	int numFBConfig;

    XSetWindowAttributes windowAttribute;
	int styleMask;
	Atom wm_delete_window_atom;
	XEvent event;
	KeySym keysym;
	int screenWidth;
	int screenHeight;
	char keys[26];


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

    // Initialize Vulkan
    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan X11 Example";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

	// xlib specific extension
	 std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME
        };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
	createInfo.flags = 0;	// for future use
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance\n";
        return -1;
    }

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
				std::cout << "API Version \t\t: " << deviceProperties.apiVersion << "\n";
				std::cout << "Device ID \t\t: " << deviceProperties.deviceID << "\n";
				std::cout << "Device Name \t\t: " << deviceProperties.deviceName << "\n";
				std::cout << "Device Type \t\t: " << deviceProperties.deviceType << "\n";
				std::cout << "Driver Version \t\t: " << deviceProperties.driverVersion << "\n";
				std::cout << "Vendor ID \t\t: " << deviceProperties.vendorID << "\n";
				g_PhysicalDevice = dev;
				break;
			}
		}


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
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
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
	// 	std::cout << qp.minImageTransferGranularity.width << qp.minImageTransferGranularity.height << qp.minImageTransferGranularity.depth << "\n";
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

	const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,		// sType
		nullptr,										// pNext
		0,												// flags
		0,												// queueFamilyIndex
		1,												// queueCount
		nullptr											// pQueuePriorities
	};

	VkDeviceCreateInfo logicalDeviceCreateInfo = {};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = nullptr;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = 1;
	logicalDeviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	logicalDeviceCreateInfo.enabledExtensionCount = 0;
	logicalDeviceCreateInfo.ppEnabledExtensionNames = nullptr;
	logicalDeviceCreateInfo.pEnabledFeatures = &requiredFeatures;

	if (vkCreateDevice(g_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &g_LogicalDevice) != VK_SUCCESS)
	{
		std::cout << "Failed to create Logical Device\n";
		return -1;
	}

	VkXlibSurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.dpy = display;
	surfaceCreateInfo.window = window;

	// Create a Vulkan Surface
	VkSurfaceKHR surface;
	if (vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) {
		std::cerr << "Failed to create Vulkan surface\n";
		vkDestroyInstance(instance, nullptr);
		return -1;
	}

	// Query the instance-level layers and extensions
	queryInstanceLayersAndExtensions();


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
				// bActiveWindow = True;
				break;

			case FocusOut:
				// bActiveWindow = True;
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
    }

    // Cleanup
	if (vkDeviceWaitIdle(g_LogicalDevice) == VK_SUCCESS)
		vkDestroyDevice(g_LogicalDevice, nullptr);
		
	vkDestroySurfaceKHR(instance, surface, nullptr); 
    vkDestroyInstance(instance, nullptr);
    XDestroyWindow(display, window);
    XFreeColormap(display, colorMap);
    XCloseDisplay(display);
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