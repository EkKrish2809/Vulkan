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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory.h> // for memset()
#include <iostream>
#include <vector>
#include <fstream>
#include <array>
#include <memory>

#include <chrono> // for timer

// for loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

// for creating sphere vertex data
#include "sphere.h"

// window size
#define WIN_WIDTH 800
#define WIN_HEIGHT 580

// window related variables
Display *display = NULL;
XVisualInfo *visualInfo = NULL;
Colormap colorMap;
Window window;
Bool fullscreen = False;
Bool b_CloseLoop = False;
Bool bActiveWindow = False;

uint32_t winWidth = 0;
uint32_t winHeight = 0;

// Log file creation 
FILE *VkLogFile = NULL;
const char* vkLogFileName = "VkApp_Log.txt";

// Vulkan related veriables
VkInstance instance;
VkPhysicalDevice g_PhysicalDevice;
VkDevice g_LogicalDevice;
VkFormat g_DepthFormat;
VkSurfaceKHR surface;
VkSurfaceFormatKHR g_SwapChainSurfaceFormat;
VkSwapchainKHR g_SwapChain = VK_NULL_HANDLE;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
VkCommandPool g_CommandPool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> g_CommandBuffer;
uint32_t g_CommandBufferCount = 0; // number of command buffers should be equal to the number of swapchain images
VkQueue g_Queue = VK_NULL_HANDLE;
VkSemaphore g_RenderCompleteSem = VK_NULL_HANDLE;
VkSemaphore g_PresentCompleteSem = VK_NULL_HANDLE;
VkRenderPass g_RenderPass = VK_NULL_HANDLE;
std::vector<VkFramebuffer> g_Framebuffers; // framebuffers for each swapchain image
VkPipelineLayout g_PipelineLayout;
VkPipeline g_GraphicsPipelinePF = VK_NULL_HANDLE;
VkPipeline g_GraphicsPipelinePV = VK_NULL_HANDLE;

VkBuffer g_VertexBuffer = VK_NULL_HANDLE;
VkDeviceMemory g_VertexBufferMemory = VK_NULL_HANDLE;
VkBuffer g_IndexBuffer = VK_NULL_HANDLE;
VkDeviceMemory g_IndexBufferMemory = VK_NULL_HANDLE;

// for uniform buffers
std::vector<VkBuffer> g_UniformBuffers;
std::vector<VkDeviceMemory> g_UniformBuffersMemory;
std::vector<void*> g_UniformBuffersMapped;

const int MAX_FRAMES_IN_FLIGHT = 3;

VkDescriptorSetLayout g_DescriptorSetLayot;
VkDescriptorPool g_DescriptorPool;
std::vector<VkDescriptorSet> g_DescriptorSets;

// for texture
VkImage g_TextureImage;
VkDeviceMemory g_TextureImageMemory;
VkImageView g_TextureImageView;
float g_MaxAnisotropyForTexture = 0.0f;
VkSampler g_TextureSampler;

// for depth
VkImage g_DepthImage;
VkDeviceMemory g_DepthImageMemory;
VkImageView g_DepthImageView;

// uint32_t imageIndex = -1;
uint32_t currentFrame = 0;



std::vector<Vertex> vertices;
std::vector<uint16_t> indices;

std::unique_ptr<Sphere> mySphere = nullptr;
// Uniform Buffer to pass the transformations to Vertex shader
struct UniformBufferObject
{
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	// for diffuse light
	glm::vec4 lightAmbient;
	glm::vec4 lightDiffuse;
	glm::vec4 lightSpecular;
	glm::vec4 lightPosition;

	glm::vec4 materialAmbient;
	glm::vec4 materialDiffuse;
	glm::vec4 materialSpecular;
	float materialShininess;

	bool lightingEnabledUniform;
};

bool bLight = false;
bool bPerVertexPipeline = false;

glm::vec4 lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
glm::vec4 lightDiffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
glm::vec4 lightSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
glm::vec4 lightPosition = glm::vec4(100.0f, 100.0f, 100.0f, 1.0f);

glm::vec4 materialAmbient = glm::vec4(0.0f, 0.0f, 0.0f ,1.0f);
glm::vec4 materialDiffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
glm::vec4 materialSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
float materialShininess = 128.0f;

// ============================== for debugging ===========================================
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
		if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			fprintf(VkLogFile, "\t%llx \n", pCallbackData->pObjects[i].objectHandle);
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
	bool initialize(void);
	void uninitialize(void);
	void update(void);
	void render(void);
	void resize(uint32_t width, uint32_t height);

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
    int screen = DefaultScreen(display);
    // int screen = XDefaultScreen(display);
	
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
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // for debugging
			// VK_EXT_DEBUG_REPORT_EXTENSION_NAME, // for debugging but deprecated
			// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			// VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME
        };

	// enable validation layers if needed
	std::vector<const char*> validationLayers = {
		 "VK_LAYER_KHRONOS_validation" ,
		//  "VK_LAYER_RENDERDOC_Capture"
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
	else 
	{
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
				// std::cout << "We will be using this Physical Device -> " << dev << "\n";
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

				// set the sampler anisotropy to true
				deviceFeatures.samplerAnisotropy = VK_TRUE;

				// get the max anisotropy fro the dev property to use for textures
				g_MaxAnisotropyForTexture = deviceProperties.limits.maxSamplerAnisotropy;

				// find the depth format for the selected physical device
				VkFormat depthFormats[] = {
					VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM	
				}; 

				for (int i=0; i < sizeof(depthFormats)/sizeof(depthFormats[0]); i++)
				{
					VkFormatProperties formatProperties;
					vkGetPhysicalDeviceFormatProperties(g_PhysicalDevice, depthFormats[i], &formatProperties);

					VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
					if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures 
						& VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
					{
						g_DepthFormat = depthFormats[i];
					}
				}

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

	// get Physical Device Memory Properties
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {0};
	vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &deviceMemoryProperties);


	// Query the Device for its queue families
	uint32_t queueFamilyPropertyCount = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties = {}; 
	vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queueFamilyPropertyCount, nullptr);
	
	queueFamilyProperties.resize(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());


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

	vkGetDeviceQueue(g_LogicalDevice, 0, 0, &g_Queue);
	// call to initialize
	initialize();


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
				case 'L':
				case 'l':
					if (bLight == false)
					{
						bLight = true;
					}
					else
					{
						bLight = false;
					}
					break;  
				case 'v':          		
				case 'V':
					bPerVertexPipeline = true;
					break;
				case 'p':
				case 'P':
					bPerVertexPipeline = false;
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
				winWidth = event.xconfigure.width;
				winHeight = event.xconfigure.height;
				resize(winWidth, winHeight); // LOWARD & HIWORD
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
	uninitialize();
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
void createSwapChain(uint32_t width, uint32_t height)
{
	// function declaration 
	VkImageView createImageView(VkImage , VkFormat , VkImageAspectFlagBits);

	// code
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
		// std::cout << "\n######### Physical Device Surface Present Modes #########\n";
		// for (const auto& mode : presentModes)
		// {
		// 	switch (mode)
		// 	{
		// 	case VK_PRESENT_MODE_IMMEDIATE_KHR:
		// 		std::cout << "VK_PRESENT_MODE_IMMEDIATE_KHR\n";
		// 		break;
		// 	case VK_PRESENT_MODE_MAILBOX_KHR:
		// 		std::cout << "VK_PRESENT_MODE_MAILBOX_KHR\n";
		// 		break;
		// 	case VK_PRESENT_MODE_FIFO_KHR:
		// 		std::cout << "VK_PRESENT_MODE_FIFO_KHR\n";
		// 		break;
		// 	case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
		// 		std::cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR\n";
		// 		break;
		// 	default:
		// 		std::cout << "Unknown Present Mode\n";
		// 		break;
		// 	}
		// }
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
		// std::cout << "\n######### Physical Device Surface Formats #########\n";
		for (int i=0; i < numSurfaceFormats; i++)
		{
			if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				g_SwapChainSurfaceFormat = surfaceFormats[i]; // choose this format
				// std::cout << "Format: " << surfaceFormats[i].format << ", Color Space: " << surfaceFormats[i].colorSpace << "\n";
			}
		}
		
	}

	// surfaceCapabilities.currentExtent.width = width;
	// surfaceCapabilities.currentExtent.height = height;
	winWidth = surfaceCapabilities.currentExtent.width;
	winHeight = surfaceCapabilities.currentExtent.height;

	// fill the swapchain create info struct
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

	swapchainImageViews.resize(swapchainImagesCount);

	// create image views for the swapchain images
	for (uint32_t i = 0; i < swapchainImagesCount; i++)
	{
		swapchainImageViews.at(i) = createImageView(swapchainImages.at(i), g_SwapChainSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlagsBit)
{
	VkImageViewCreateInfo imageViewCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image, // set this member in the loop
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = 
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask = aspectFlagsBit,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}

	};

	VkImageView imageView;
	if (vkCreateImageView(g_LogicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		std::cerr << "Failed to create imageView for swapchain image\n";
		return VK_NULL_HANDLE;
	}

	return imageView;
}

void createCommandPool()
{
	// create command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		// .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = 0
	};

	if (vkCreateCommandPool(g_LogicalDevice, &commandPoolCreateInfo, nullptr, &g_CommandPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create command pool\n";
		return;
	}
}

// create the command pool and command buffers
void createCommandBuffers()
{
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
	VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f}; // black color
	std::array<VkClearValue, 2> clearValue;
	clearValue[0].color = clearColor;
	clearValue[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = g_RenderPass,
		.renderArea =
		{
			.offset = {.x = 0, .y = 0},
			.extent = {winWidth, winHeight}
		},
		.clearValueCount = static_cast<uint32_t>(clearValue.size()),
		.pClearValues = clearValue.data()
	};

	for (uint32_t i=0; i < g_CommandBuffer.size(); i++)
	{
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

		renderPassBeginInfo.framebuffer = g_Framebuffers.at(i);

		// begin rendr pass
		vkCmdBeginRenderPass(g_CommandBuffer.at(i), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// begin the graphics pipeline
		if (bPerVertexPipeline == true)
			vkCmdBindPipeline(g_CommandBuffer.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, g_GraphicsPipelinePV);
		else
			vkCmdBindPipeline(g_CommandBuffer.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, g_GraphicsPipelinePF);

		// reset the viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(winWidth);
		viewport.height = static_cast<float>(winHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(g_CommandBuffer.at(i), 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = {static_cast<uint32_t>(winWidth), static_cast<uint32_t>(winHeight)};
		vkCmdSetScissor(g_CommandBuffer.at(i), 0, 1, &scissor);

		// record the commands for rendering here
		// Draw command
		VkBuffer vertexBuffers[] = {g_VertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(g_CommandBuffer.at(i), 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(g_CommandBuffer.at(i), g_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		// bind the descriptor sets
		vkCmdBindDescriptorSets(g_CommandBuffer.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, &g_DescriptorSets.at(currentFrame), 0, nullptr);
		
		// vkCmdDraw(g_CommandBuffer.at(i), static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(g_CommandBuffer.at(i), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// end the render pass
		vkCmdEndRenderPass(g_CommandBuffer.at(i));

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
	// vkGetDeviceQueue(g_LogicalDevice, 0, 0, &g_Queue);

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
		std::cerr << "Failed to create present complete semaphore\n";
		return;
	}
}

// create render-pass
void createRenderPass()
{
	// function declaration
	VkFormat findDepthFormat(void);

	VkAttachmentDescription colorAttachment = 
	{
		.flags = 0,
		.format = g_SwapChainSurfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT, // no multisampling for now
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the attachment before rendering
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE, // store the attachment after rendering
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // no stencil
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef = 
	{
		.attachment = 0, // index of the attachment in the render pass
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout of the attachment
	};

	VkAttachmentDescription depthAttachment =
	{
		.flags = 0,
		.format = findDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef = 
	{
		.attachment = 1, // index of the attachment in the render pass
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // layout of the attachment
	};

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, // subpass is for graphics pipeline
		.inputAttachmentCount = 0, // no input attachments
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1, // one color attachment
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
		
	};

	// add subpass dependency 
	VkSubpassDependency subpassDependency =
	{
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(colorAttachment);
	attachments.push_back(depthAttachment); // use this once depth buffer is implemented

	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = (uint32_t)attachments.size(),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &subpassDependency
	};

	if (vkCreateRenderPass(g_LogicalDevice, &renderPassCreateInfo, nullptr, &g_RenderPass) != VK_SUCCESS)
	{
		std::cerr << "Failed to create render pass\n";
		return;
	}

}

// create the framebuffer for each swapchain image
void createFramebuffers(uint32_t width, uint32_t height)
{
	// VkBuffer createVulkanBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	g_Framebuffers.resize(swapchainImages.size());

	for (int i = 0; i < swapchainImages.size(); i++)
	{
		std::vector<VkImageView> imageViewAttachments;
		imageViewAttachments.push_back(swapchainImageViews.at(i)); 

		// use depth attachment if available
		imageViewAttachments.push_back(g_DepthImageView);

		VkFramebufferCreateInfo framebufferCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = g_RenderPass,
			.attachmentCount = (uint32_t)imageViewAttachments.size(),
			.pAttachments = imageViewAttachments.data(),
			.width = width,
			.height = height,
			.layers = 1
		};

		if (vkCreateFramebuffer(g_LogicalDevice, &framebufferCreateInfo, nullptr, &g_Framebuffers.at(i)) != VK_SUCCESS)
		{
			std::cerr << "Failed to create framebuffer for swapchain image at index " << i << "\n";
			return;
		}
	}
}

// ======================== for Shader Loading and Compilation ========================
std::vector<char> readFile(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cerr << "Failed to open file: " << filename << "\n";
		return {};
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void createGraphicsPipeline()
{
	// function decleration
	std::vector<char> readFile(std::string filename);
	VkVertexInputBindingDescription getBindingDescription(void);
	std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions(void);
	VkShaderModule createShaderModule(const std::vector<char> &shaderCode);

	// code
	// ===================== PER VERTEX ==============================//
	std::cout << "PER VERTEX\n";
	auto vertexShaderCode = readFile("shaders/bin/lightPV.vert.spv");
	auto fragmentShaderCode = readFile("shaders/bin/lightPV.frag.spv");

	if (vertexShaderCode.empty() || fragmentShaderCode.empty())
	{
		std::cerr << "Failed to read shader files\n";
		return;
	}
	std::cout << "Vertex Shader Code Size: " << vertexShaderCode.size() << "\n";
	std::cout << "Fragment Shader Code Size: " << fragmentShaderCode.size() << "\n";

	// create shader modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	if (vertexShaderModule == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to create vertex shader module\n";
		return;
	}

	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
	if (fragmentShaderModule == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to create fragment shader module\n";
		return;
	}

	// shader stage for vertex shader
	VkPipelineShaderStageCreateInfo vertexShaderStageInfoPV =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShaderModule,
		.pName = "main"
	};

	// shader stage for vertex shader
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfoPV =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStagesPV[] = {vertexShaderStageInfoPV, fragmentShaderStageInfoPV};

	// ===================== PER FRAGMENT ==============================//
	std::cout << "PER FRAGMENT\n";
	// reinitialize the variables
	vertexShaderModule = VK_NULL_HANDLE;
	fragmentShaderModule = VK_NULL_HANDLE;

	// read binary SPIR-V shaders
	vertexShaderCode = readFile("shaders/bin/lightPF.vert.spv");
	fragmentShaderCode = readFile("shaders/bin/lightPF.frag.spv");

	if (vertexShaderCode.empty() || fragmentShaderCode.empty())
	{
		std::cerr << "Failed to read shader files\n";
		return;
	}
	std::cout << "Vertex Shader Code Size: " << vertexShaderCode.size() << "\n";
	std::cout << "Fragment Shader Code Size: " << fragmentShaderCode.size() << "\n";

	// create shader modules
	vertexShaderModule = createShaderModule(vertexShaderCode);
	if (vertexShaderModule == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to create vertex shader module\n";
		return;
	}

	fragmentShaderModule = createShaderModule(fragmentShaderCode);
	if (fragmentShaderModule == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to create fragment shader module\n";
		return;
	}

	// shader stage for vertex shader
	VkPipelineShaderStageCreateInfo vertexShaderStageInfoPF =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShaderModule,
		.pName = "main"
	};

	// shader stage for vertex shader
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfoPF =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShaderModule,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shaderStagesPF[] = {vertexShaderStageInfoPF, fragmentShaderStageInfoPF};

	
	// ===================================================================================================
	VkVertexInputBindingDescription bindingDescription = getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = getAttributeDescriptions();

	// Vertex input state
	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions = attributeDescriptions.data()
	};

	// Input assembly state ; set the topology for geometry
	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		.primitiveRestartEnable = VK_FALSE
	};
	
	// viewport and scissor states
	VkViewport viewport = 
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float) WIN_WIDTH,
		.height = (float) WIN_HEIGHT,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = {static_cast<uint32_t>(WIN_WIDTH), static_cast<uint32_t>(WIN_HEIGHT)}
	};

	// dynamic states
	std::vector<VkDynamicState> dynamicStates = 
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data()
	};

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizer =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};


	// multisample state
	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};

	// depth-stencil state will be added here
	// adding depth-stencil info to the pipeline
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};

	// color blending state
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.colorBlendOp = VK_BLEND_OP_ADD, // Optional
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.alphaBlendOp = VK_BLEND_OP_ADD, // Optional
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY, // Optional
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // Optional
	};
	
	
	// pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &g_DescriptorSetLayot,
		.pushConstantRangeCount = 0, // Optional
		.pPushConstantRanges = nullptr // Optional
	};

	if (vkCreatePipelineLayout(g_LogicalDevice, &pipelineLayoutInfo, nullptr, &g_PipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	
	// ================ PER VERTEX =================
	// graphics pipeline create info
	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2, // vertex and fragment shader states
		.pStages = shaderStagesPV,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = nullptr,  // if tessellation is used
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencilCreateInfo, // if depth-stencil is used
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = g_PipelineLayout,
		.renderPass = g_RenderPass,
		.subpass = 0, // index of the subpass in the render pass
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1

	};

	if (vkCreateGraphicsPipelines(g_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &g_GraphicsPipelinePV) != VK_SUCCESS)
	{
		std::cerr << "Failed to create graphics pipeline\n";
		return;
	}


	// ================ PER FRAGMENT ===========================
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStagesPF;

	// create per fragment pipeline
	if (vkCreateGraphicsPipelines(g_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &g_GraphicsPipelinePF) != VK_SUCCESS)
	{
		std::cerr << "Failed to create graphics pipeline\n";
		return;
	}

	// This the last step after the graphics pipeline is created
	// destroy the shader modules after use
	vkDestroyShaderModule(g_LogicalDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(g_LogicalDevice, fragmentShaderModule, nullptr);
}

VkShaderModule createShaderModule(const std::vector<char>& shaderCode)
{
	VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.codeSize = shaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*> (shaderCode.data()),
	};

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(g_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cerr << "Failed to create shader module\n";
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

VkVertexInputBindingDescription getBindingDescription() 
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() 
{
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	// position attribute
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, positions);

	// color attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normals);

    return attributeDescriptions;
}

// create Index buffer
void createIndexBuffer()
{
	// function declearation
	void createVulkanBuffers(VkDeviceSize , VkBufferUsageFlags , VkMemoryPropertyFlags , VkBuffer &, VkDeviceMemory &, VkSharingMode);
	uint32_t findMemoryTypes(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// create vertex buffer
	VkDeviceSize bufSize = sizeof(indices[0]) * indices.size();
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createVulkanBuffers(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory, VK_SHARING_MODE_EXCLUSIVE);
		
	// filling the vertex buffer
	void *data;
	vkMapMemory(g_LogicalDevice, stagingBufferMemory, 0, bufSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufSize);
	vkUnmapMemory(g_LogicalDevice, stagingBufferMemory);
		
	createVulkanBuffers(bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		g_IndexBuffer, g_IndexBufferMemory, VK_SHARING_MODE_EXCLUSIVE);

	copyBuffer(stagingBuffer, g_IndexBuffer, bufSize);

	vkDestroyBuffer(g_LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(g_LogicalDevice, stagingBufferMemory, nullptr);
}

// create vertex buffer
void createVertexBuffer()
{
	// function declearation
	void createVulkanBuffers(VkDeviceSize , VkBufferUsageFlags , VkMemoryPropertyFlags , VkBuffer &, VkDeviceMemory &, VkSharingMode);
	uint32_t findMemoryTypes(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	// void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// create vertex buffer
	VkDeviceSize bufSize = sizeof(vertices[0]) * vertices.size();
	
	// VkBuffer stagingBuffer;
	// VkDeviceMemory stagingBufferMemory;
	createVulkanBuffers(bufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		g_VertexBuffer, g_VertexBufferMemory, VK_SHARING_MODE_EXCLUSIVE);
		
	// filling the vertex buffer
	void *data;
	vkMapMemory(g_LogicalDevice, g_VertexBufferMemory, 0, bufSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufSize);
	vkUnmapMemory(g_LogicalDevice, g_VertexBufferMemory);
		
	// createVulkanBuffers(bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
	// 	g_VertexBuffer, g_VertexBufferMemory, VK_SHARING_MODE_EXCLUSIVE);

	// copyBuffer(stagingBuffer, g_VertexBuffer, bufSize);

	// vkDestroyBuffer(g_LogicalDevice, stagingBuffer, nullptr);
	// vkFreeMemory(g_LogicalDevice, stagingBufferMemory, nullptr);
}

// create uniform buffers
void createUniformBuffers()
{
	// function declaration
	void createVulkanBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory, VkSharingMode sharingMode);

	// code
	VkDeviceSize bufSize = sizeof(UniformBufferObject);

	g_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	g_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
	g_UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		createVulkanBuffers(bufSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
							g_UniformBuffers[i], g_UniformBuffersMemory[i], VK_SHARING_MODE_EXCLUSIVE);
		vkMapMemory(g_LogicalDevice, g_UniformBuffersMemory[i], 0, bufSize, 0, &g_UniformBuffersMapped[i]);
	}
}

// as we can not do 'vkMapMemory()' during Staging buffer methode, we need a special type of memory transferring mechanism
// hence, below function
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// function declaration
	VkCommandBuffer beginSingleTimeCommand(void);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer);

	// begin recording
	VkCommandBuffer commandBuffer = beginSingleTimeCommand();

	VkBufferCopy copyRegion =
	{
		.srcOffset = 0, // optional
		.dstOffset = 0, // optional
		.size = size
	};

	// copy the buffer
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	// end recording
	endSingleTimeCommands( commandBuffer);

}

VkCommandBuffer beginSingleTimeCommand()
{
	VkResult result;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	// memory transferring is done using Command buffers, just like Drawing commands
	VkCommandBufferAllocateInfo cmdBufAllocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = g_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	
	result = vkAllocateCommandBuffers(g_LogicalDevice, &cmdBufAllocInfo, &commandBuffer);
	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to Allocate Command buffer for " << result << "reason\n";
		return VK_NULL_HANDLE;
	}

	// start recording
	VkCommandBufferBeginInfo cmdBufBeginInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// usage flag for one time submit
	};

	if (vkBeginCommandBuffer(commandBuffer, &cmdBufBeginInfo) != VK_SUCCESS)
	{
		std::cerr << "Failed to Begin Command Buffer\n";
		return VK_NULL_HANDLE;
	}

	return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer cmdBuffer)
{
	// now that the copying is done, end the command buffer
	if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
	{
		std::cerr << "Failed to end the CommandBuffer\n";
		return;
	}


	// now execute the CommandBuffer to complete thr Buffer Transfer
	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuffer
	};

	if (vkQueueSubmit(g_Queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		std::cerr << "Failed to submit the Queue\n";
		return;
	}

	vkQueueWaitIdle(g_Queue);

	vkFreeCommandBuffers(g_LogicalDevice, g_CommandPool, 1, &cmdBuffer);
}

// find memory requirements to allocate the memory for Vertex Buffers
uint32_t findMemoryTypes(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	// query the available memory
	VkPhysicalDeviceMemoryProperties memoryProperties;

	vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &memoryProperties);

	// find the memory type that is suitable for the buffer itself
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) 
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

// create vulkan buffers
void createVulkanBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, 
							VkDeviceMemory &bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE)
{
	// local function declaration
	uint32_t findMemoryTypes(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	// code
	VkBufferCreateInfo bufferCreateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 									// sType
		.pNext = nullptr,                               								// pNext
		.flags = 0,                                     								// flags
		.size = size,                                     								// size can be a parameter
		.usage = usage,   																// usage can be decided based on usecase ((as a parameter))
		.sharingMode = sharingMode,             										// sharingMode
		.queueFamilyIndexCount = 0,                                     				// queueFamilyIndexCount
		.pQueueFamilyIndices = nullptr                                					// pQueueFamilyIndices
	};

	// create the buffer
	if (vkCreateBuffer(g_LogicalDevice, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		std::cerr << "Failed to create Vulkan buffer\n";
		return;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(g_LogicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = findMemoryTypes(memoryRequirements.memoryTypeBits, properties)
	};

	if (vkAllocateMemory(g_LogicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate the memory for the buffer\n";
		return;
	}

	vkBindBufferMemory(g_LogicalDevice, buffer, bufferMemory, 0);

}


// passing uniforms to the shader
void createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding =
	{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr, // for image sampling (textures)
	};

	VkDescriptorSetLayoutBinding samplerLayoutBinding =
	{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr,
	};

	std::array<VkDescriptorSetLayoutBinding, 2> shaderBindings = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t> (shaderBindings.size()),
		.pBindings = shaderBindings.data()
	};

	if (vkCreateDescriptorSetLayout(g_LogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &g_DescriptorSetLayot) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout\n";
		return;
	}
}


// create descriptor pool
void createDescriptorPool()
{
	// descriptor pool size
	std::array<VkDescriptorPoolSize, 2> descriptorPoolSize{};
	descriptorPoolSize[0] =
	{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t> (MAX_FRAMES_IN_FLIGHT)
	};

	descriptorPoolSize[1] =
	{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = static_cast<uint32_t> (MAX_FRAMES_IN_FLIGHT)
	};

	/*
		Validation layer may not catch the error of Inadequate descriptor pool size
	*/
	// fill the descriptor pool info
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t> (MAX_FRAMES_IN_FLIGHT), // max number of descriptor sets that may be allocated
		.poolSizeCount = static_cast<uint32_t> (descriptorPoolSize.size()),
		.pPoolSizes = descriptorPoolSize.data(),
	};

	// create descriptor pool
	if (vkCreateDescriptorPool(g_LogicalDevice, &descriptorPoolCreateInfo, nullptr, &g_DescriptorPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor pool\n";
		return;
	}

}

// create descriptor set
void createDescriptorSet()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, g_DescriptorSetLayot);
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = g_DescriptorPool,
		.descriptorSetCount = static_cast<uint32_t> (MAX_FRAMES_IN_FLIGHT),
		.pSetLayouts = descriptorSetLayouts.data()
	};

	g_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(g_LogicalDevice, &descriptorSetAllocateInfo, g_DescriptorSets.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate descriptor sets\n";
		return;
	}

	// configure the descriptors
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo descBufferInfo =
		{
			.buffer = g_UniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		// for texture
		// VkDescriptorImageInfo descImageInfo =
		// {
		// 	.sampler = g_TextureSampler,
		// 	.imageView = g_TextureImageView,
		// 	.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

		// };
		
		std::array<VkWriteDescriptorSet, 1> writeDescriptorSet{};
		writeDescriptorSet[0] =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = g_DescriptorSets.at(i),
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = nullptr, // optional for now (used for Image data)
			.pBufferInfo = &descBufferInfo, // (used for buffers)
			.pTexelBufferView = nullptr // optional for now (used for buffer views)
		};

		// writeDescriptorSet[1] =
		// {
		// 	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		// 	.dstSet = g_DescriptorSets.at(i),
		// 	.dstBinding = 1,
		// 	.dstArrayElement = 0,
		// 	.descriptorCount = 1,
		// 	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		// 	.pImageInfo = &descImageInfo, // optional for now (used for Image data)
		// 	.pBufferInfo = nullptr, // (used for buffers)
		// 	.pTexelBufferView = nullptr // optional for now (used for buffer views)
		// };

		vkUpdateDescriptorSets(g_LogicalDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, nullptr);		
	}

}

// ============================== Texture Related ============================
// for loading textures
void createTextureImage()
{
	// function declaration
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, 
		VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	// local variables
	int texWidth, texHeight, nChannels;

	stbi_uc *pixels = stbi_load("./textures/fire.png", &texWidth, &texHeight, &nChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		std::cerr << "Failed to load image\n";
		return;
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// create buffer for image, just like we did for vertex buffer
	createVulkanBuffers(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory, VK_SHARING_MODE_EXCLUSIVE);

	// copy the image data to the buffer
	void *data;
	vkMapMemory(g_LogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(g_LogicalDevice, stagingBufferMemory);

	// free the loaded pixel data
	stbi_image_free(pixels);

	// create the image and image memory
	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, g_TextureImage, g_TextureImageMemory);

	// transition the image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	transitionImageLayout(g_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// perform buffer to image copy operation
	copyBufferToImage(stagingBuffer, g_TextureImage, texWidth, texHeight);

	// To be able to start sampling from the texture image in the shader, we need one last transition to prepare it for shader access
	transitionImageLayout(g_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// destroy staging buffer and buffer memory
	vkDestroyBuffer(g_LogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(g_LogicalDevice, stagingBufferMemory, nullptr);
}

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, 
	VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &imageMemory)
{
	// function delcaration
	uint32_t findMemoryTypes(uint32_t typeFilter, VkMemoryPropertyFlags properties);


	// set the configurations of the texture buffer we are creating
	VkImageCreateInfo imageCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = {width, height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // as we are using this image as a destination to copy texel data from texture
	};

	// register the empty texture to be created with above filled configuration
	if (vkCreateImage(g_LogicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
	{
		std::cerr << "Failed to create image\n";
		return;
	}

	// find the memory requirements for the above registered empty image to be allocated
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(g_LogicalDevice, image, &memoryRequirements);

	// fill the memory allocation configuration struct with above acquired memory size
	VkMemoryAllocateInfo memoryAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = findMemoryTypes(memoryRequirements.memoryTypeBits, propertyFlags)
	};

	// allocate the memory
	if (vkAllocateMemory(g_LogicalDevice, &memoryAllocateInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate memory to image\n";
		return;
	}

	// bind the allocated image memory with the image itself
	vkBindImageMemory(g_LogicalDevice, image, imageMemory, 0);
}

// function to handle the image layout transition
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
{
	// function declaration
	VkCommandBuffer beginSingleTimeCommand(void);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer);
	bool hasStencilComponent(VkFormat );

	// code
	VkCommandBuffer commandBuffer = beginSingleTimeCommand();

	VkImageMemoryBarrier imageMemBarrier = 
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		// .srcAccessMask = 0, // TO DO
		// .dstAccessMask = 0, // TO DO
		.oldLayout = oldImageLayout,
		.newLayout = newImageLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,	// used when transferring queueFamily ownership (from), no need in this case
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,	// used when transferring queueFamily ownership (to), no need in this case
		.image = image,
		.subresourceRange = 
		{
			// .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};

	// decide the aspect mask based on newImageLayout
	if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
		{
			imageMemBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		imageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage, destinationStage;

	if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemBarrier.srcAccessMask = 0;
		imageMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemBarrier.srcAccessMask = 0;
		imageMemBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		std::cerr << "Invalid argument\n";
	}

	vkCmdPipelineBarrier(commandBuffer,
					sourceStage,
					destinationStage,
					0,
					0, nullptr,				// memory barried
					0, nullptr,				// buffer-memory barrier
					1, &imageMemBarrier);	// image-memory barrier

	endSingleTimeCommands(commandBuffer);

}

// for copying buffer to image
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	// function declaration
	VkCommandBuffer beginSingleTimeCommand(void);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer);

	// code
	VkCommandBuffer commandBuffer = beginSingleTimeCommand();

	VkBufferImageCopy bufferImageCopy =
	{
		.bufferOffset = 0,
		.bufferRowLength = 0, 
		.bufferImageHeight = 0,
		.imageSubresource =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset =
		{
			0, 0, 0
		},
		.imageExtent =
		{
			width,
			height,
			1
		}
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

	endSingleTimeCommands(commandBuffer);
}

// create texture view
void createTextureImageView()
{
	// function declaration
	VkImageView createImageView(VkImage , VkFormat , VkImageAspectFlagBits);

	// create texture Image View
	g_TextureImageView = createImageView(g_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

}

// setup the sampler object
void createTextureSampler()
{
	// configure the sampler
	VkSamplerCreateInfo samplerCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = g_MaxAnisotropyForTexture,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,

	};

	// create texture sampler
	if (vkCreateSampler(g_LogicalDevice, &samplerCreateInfo, nullptr, &g_TextureSampler) != VK_SUCCESS)
	{
		std::cerr << "Failed to create texture sampler\n";
		return;
	}
}

// ================================ For Depth =============================
void createDepthResources()
{
	// function declaration
	VkImageView createImageView(VkImage , VkFormat , VkImageAspectFlagBits );
	VkFormat findDepthFormat(void);
	bool hasStencilComponent(VkFormat );
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, 
		VkMemoryPropertyFlags propertyFlags, VkImage &image, VkDeviceMemory &imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);

	VkFormat depthFormat = findDepthFormat();

	// create depth image and depth image view
	createImage(winWidth, winHeight, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, g_DepthImage, g_DepthImageMemory);

	g_DepthImageView = createImageView(g_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(g_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

// find supported format for Depth
VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(g_PhysicalDevice, format, &formatProperties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format");
}

VkFormat findDepthFormat()
{
	// function declaration
	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

// check for stencil component 
bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// resize the swapchain
void resize(uint32_t width, uint32_t height)
{
	// destroy the old swapchain
	if (g_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(g_LogicalDevice, g_SwapChain, nullptr);
		g_SwapChain = VK_NULL_HANDLE;
	}

	// vkDeviceWaitIdle(g_LogicalDevice); // wait for device to finish current rendering operations

	// recreate swapchain with new width and height
	createSwapChain(width, height);

	// create renderpass and framebuffers
	createRenderPass();
	createDepthResources();
	createFramebuffers(winWidth, winHeight);

	// createCommandBuffers();
	// recordCommandBuffers();
}


bool initialize()
{
	// function declaration
	void queryInstanceLayersAndExtensions(void);
	void createSwapChain(uint32_t width, uint32_t height);
	void createCommandPool(void);
	void createCommandBuffers(void);
	void recordCommandBuffers(void);
	void initializeQueue(void);
	void createRenderPass(void);
	void createFramebuffers(uint32_t, uint32_t);
	void createGraphicsPipeline(void);
	void createVertexBuffer(void);
	void createUniformBuffers(void);
	void createDescriptorSetLayout(void);
	void createDescriptorPool(void);
	void createDescriptorSet(void);
	void createTextureImage(void);
	void createDepthResources(void);
    
	// Query the instance-level layers and extensions
	queryInstanceLayersAndExtensions();

	// create command pool to be used while creating command buffers
	createCommandPool();

	// create the swapchain
	createSwapChain(WIN_WIDTH, WIN_HEIGHT);
	
	// create renderpass and framebuffers
	createRenderPass();
	
	// create descriptor set layour
	createDescriptorSetLayout();

	// ============ Initialize the Sphere ==============
	mySphere = std::make_unique<Sphere>();
	mySphere->initSphere();

	vertices = mySphere->getVertexData();
	indices = mySphere->getIndexData();

	// create the pipeline layout
	createGraphicsPipeline();
	
	// adding depth
	createDepthResources();
	
	createFramebuffers(winWidth, winHeight);
	
	// create texture image
	// createTextureImage();
	// createTextureImageView();
	// createTextureSampler();

	// create vertex buffer
	createVertexBuffer();
	// using Index Buffer instead
	createIndexBuffer();
	// create uniform buffer
	createUniformBuffers();

	// create descriptor pool
	createDescriptorPool();
	// create descriptor sets
	createDescriptorSet();

	// create command buffers
	createCommandBuffers();

	// record command buffers
	recordCommandBuffers();

	// initialize the queue for rendering
	initializeQueue();

    return true; // Placeholder for initialization logic
}

void render()
{
	// function declaration
	void updateUniformBuffer(uint32_t );
	
	uint32_t imageIndex = -1;
	VkResult result = VK_SUCCESS;

	
	// acquire the next image from the swapchain
	result = vkAcquireNextImageKHR(g_LogicalDevice, g_SwapChain, UINT64_MAX, g_PresentCompleteSem, nullptr, &imageIndex);
	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to acquire next image from swapchain because of " << result << "\n";;
		return;
	}
	// call to record commandbuffer
	// if (bPerVertexPipeline == true)
		recordCommandBuffers();
	// if (bPerVertexPipeline == true)
	// 		vkCmdBindPipeline(g_CommandBuffer.at(imageIndex), VK_PIPELINE_BIND_POINT_GRAPHICS, g_GraphicsPipelinePV);
	// 	else
	// 		vkCmdBindPipeline(g_CommandBuffer.at(imageIndex), VK_PIPELINE_BIND_POINT_GRAPHICS, g_GraphicsPipelinePF);

	// update the uniform buffer here
	updateUniformBuffer(currentFrame);

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

	// if (vkQueueWaitIdle(g_Queue) != VK_SUCCESS) // wait for the queue to finish rendering
	// {
	// 	std::cerr << "Failed to wait for queue to finish rendering\n";
	// 	return;
	// }

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void updateUniformBuffer(uint32_t curImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	// transformation
	UniformBufferObject ubo{};
	ubo.modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * time, glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.viewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.projectionMatrix = glm::perspective(glm::radians(45.0f), winWidth / (float)winHeight, 0.1f, 100.0f);

	ubo.projectionMatrix[1][1] *= -1; 

	// if (bPerVertexPipeline == true)
	if (bLight == true)
	{
		// for diffuse light
		ubo.lightAmbient = lightAmbient;
		ubo.lightDiffuse = lightDiffuse;
		ubo.lightSpecular = lightSpecular;
		ubo.lightPosition = lightPosition;

		ubo.materialAmbient = materialAmbient;
		ubo.materialDiffuse = materialDiffuse;
		ubo.materialSpecular = materialSpecular;
		ubo.materialShininess = materialShininess;

		ubo.lightingEnabledUniform = true;
	}
	else
	{
		ubo.lightingEnabledUniform = false;
	}

	// map the UBO
	memcpy(g_UniformBuffersMapped[curImage], &ubo, sizeof(ubo));

}

void update()
{

}

void uninitialize()
{
	// destroy texture sampler
	vkDestroySampler(g_LogicalDevice, g_TextureSampler, nullptr);

	// destroy textureImageView
	vkDestroyImageView(g_LogicalDevice, g_TextureImageView, nullptr);
	// destroy texture image
	vkDestroyImage(g_LogicalDevice, g_TextureImage, nullptr);
	vkFreeMemory(g_LogicalDevice, g_TextureImageMemory, nullptr);

	// destroy the uniform buffers
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(g_LogicalDevice, g_UniformBuffers[i], nullptr);
		vkFreeMemory(g_LogicalDevice, g_UniformBuffersMemory[i], nullptr);
	}
	vkDestroyDescriptorSetLayout(g_LogicalDevice, g_DescriptorSetLayot, nullptr);

	// destroy descriptorSetLayout
	if(g_DescriptorSetLayot != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(g_LogicalDevice, g_DescriptorSetLayot, nullptr);
		g_DescriptorSetLayot = VK_NULL_HANDLE;
	}

	// free the IndexBufferMemory
	if (g_IndexBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(g_LogicalDevice, g_IndexBufferMemory, nullptr);
	}

	// destroy vertex buffer
	if (g_IndexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(g_LogicalDevice, g_IndexBuffer, nullptr);
		g_IndexBuffer = VK_NULL_HANDLE;
	}
	
	// free the VertexBufferMemory
	if (g_VertexBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(g_LogicalDevice, g_VertexBufferMemory, nullptr);
	}

	// destroy vertex buffer
	if (g_VertexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(g_LogicalDevice, g_VertexBuffer, nullptr);
		g_VertexBuffer = VK_NULL_HANDLE;
	}

	// destroy the graphics pipeline
	if (g_GraphicsPipelinePF != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(g_LogicalDevice, g_GraphicsPipelinePF, nullptr);
		g_GraphicsPipelinePF = VK_NULL_HANDLE;
	}

	// destroy the pipeline layout
	if (g_PipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(g_LogicalDevice, g_PipelineLayout, nullptr);
		g_PipelineLayout = VK_NULL_HANDLE;
	}


	// destroy swapchain image views
	// if (swapchainImageViews.size() > 0)
	{
		for (auto& imageView : swapchainImageViews)
		{
			vkDestroyImageView(g_LogicalDevice, imageView, nullptr);
			std::cout << "Destroyed Swapchain ImageVIews\n";
		}
		swapchainImageViews.clear();
	}

	// destroy render pass
	// if (g_RenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(g_LogicalDevice, g_RenderPass, nullptr);
		g_RenderPass = VK_NULL_HANDLE;
		std::cout << "Destroyed Renderpass\n";
	}

	// destroy framebuffers
	// if (g_Framebuffers.size() > 0)
	{
		for (auto& framebuffer : g_Framebuffers)
		{
			vkDestroyFramebuffer(g_LogicalDevice, framebuffer, nullptr);
			std::cout << "Destroyed Framebuffer\n";
		}
		g_Framebuffers.clear();
	}

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
}
