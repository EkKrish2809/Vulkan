// Header files
#include<windows.h>
#include<winuser.h>
#include<stdio.h>
#include<stdlib.h>
#include "Vlkn.h"

#include <vector>

// OpenGL header files
// #include<GL/gl.h>
// vulkan header files
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// OpenGL Libraries
// #pragma comment (lib,"OpenGL32.lib")
// Vulkan Libraries
#pragma comment (lib,"vulkan-1.lib")

// global function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// global variable declarations
HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;
BOOL gbFullScreen = FALSE;
FILE *gpFile = NULL;
BOOL gbActiveWindow = FALSE;

// Vulkan global variables
VkDevice device;
VkSwapchainKHR swapchain;
VkRenderPass renderPass;
VkCommandPool commandPool;
VkSurfaceKHR surface;
VkInstance instance;

// entry-point function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// function declarations
	int initialize(void);
	void display(void);
	void update(void);
	void uninitialize(void);

	// variable declarations
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szAppName[]=TEXT("MyWindow");
	BOOL bDone = FALSE;
	int iScreenWidth, iScreenHeight;
	int iRetVal = 0;

	// code
	if (fopen_s(&gpFile, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("Creation of Log file Failed. Exitting..."), TEXT("File I/O"), MB_OK);
		exit(0);
	}
	else
	{
		fprintf(gpFile, "Log file Successfully Created. \n");
	}

	
	

	// initialization of WNDCLASSEX structure
	wndclass.cbSize = sizeof(WNDCLASSEX);	// was not before 1993
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpfnWndProc = WndProc;	// ya window chya callback function cha addrss
	wndclass.hInstance = hInstance;	// 
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));	//window chya left top cha icon ani task bar cha icon
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));	// Explorer cha icon	//was not before 1993 (newly added)

	// registering above WINDCLASSEX
	RegisterClassEx(&wndclass);

	// Screen dimensions
	iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	// create the window
	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
			szAppName,
			TEXT("Vulkan Window"),
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
			(iScreenWidth/4),	//x c-ordinate 
			(iScreenHeight/4),	// y - coordinate
			WIN_WIDTH,	// width
			WIN_HEIGHT,	// height
			NULL,		// parent window handle (desktop window) = HWND_WIMDOW
			NULL,		// Menu handle
			hInstance,
			NULL);

	ghwnd = hwnd;

	// initialize
	iRetVal = initialize();

	if (iRetVal == -1)
	{
		fprintf(gpFile, "ChoosePixelFormat Failed\n");
		uninitialize();
	}

	else if (iRetVal == -2)
	{
		fprintf(gpFile, "SetPixelFormat Failed\n");
		uninitialize();
	}

	else if (iRetVal == -3)
	{
		fprintf(gpFile, "Creating OpenGL Cintext Failed\n");
		uninitialize();
	}

	else if (iRetVal == -4)
	{
		fprintf(gpFile, "Making OpenGL Context as Current Context Failed\n");
		uninitialize();
	}

	else
	{
		fprintf(gpFile, "Initializing Successfull\n");
	}

	// show window
	ShowWindow(hwnd, iCmdShow);

	// forgrounding and focusing the window
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	// Game Loop
	while (bDone == FALSE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = TRUE;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (gbActiveWindow == TRUE)
			{
				// render the scene
				display();

				// update the scene
				update();
			}
		}
	}

	uninitialize();
	return((int)msg.wParam);
}

// callback function
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// function declarations
	void ToggleFullScreen(void);
	void resize(int, int);
	

	// code
	switch(iMsg)
	{
	case WM_SETFOCUS:
		gbActiveWindow = TRUE;
		break;
	case WM_KILLFOCUS:
		gbActiveWindow = FALSE;
		break;

	case WM_ERASEBKGND:
	
		return(0);
	case WM_CHAR:
		switch (wParam)
		{
		case 'F':
		case 'f':
			ToggleFullScreen();
			break;
		default:
			break;
		}

		break;
	case WM_KEYDOWN:
        	switch (wParam)
        	{
        	case 27:
        		DestroyWindow(hwnd);
        		break;
        	default:
        		break;
        	}
        	break;
	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}


void ToggleFullScreen(void)
{
	// variable declarations
	static DWORD dwStyle;
	static WINDOWPLACEMENT wp;
	MONITORINFO mi;

	// code
	wp.length = sizeof(WINDOWPLACEMENT);

	if (gbFullScreen == FALSE)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi.cbSize = sizeof(MONITORINFO);

			if (GetWindowPlacement(ghwnd, &wp) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}

			ShowCursor(FALSE);
			gbFullScreen = TRUE;
		}
	}
	else
	{
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wp);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);
		gbFullScreen = FALSE;
	}
}

int initialize(void)
{
	// function declarations

	// variable declarations
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex = 0;

	// code
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
	// memset((void *) &pfd, NULL, sizeof(PIXELFORMATDESCRIPTOR)); used in LINUX for zeroing out the struct same as zeroMemory(..) in WINDOWS

	// initialization of PIXELFORMATDESCRIPTOR
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1; // MS supports OpenGL version 1.0 , not above versions
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; // eth kalt ki WM_PAINT ata use honar nahi. ata OpenGL cha pixel painting karel
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;

	// get DC
	ghdc = GetDC(ghwnd); // gives neutral DC which can be converted as per need .. It gives DC to fullDESKTOP if NULL is passed as parameter

	// choose Pixel Format
	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd); // convertes the pixel format to OpenGL of ghdc
	if (iPixelFormatIndex == 0)
	{
		return(-1);
	}

	// set the chosen Pixel Format (sets all 26 parameters of pfd which ChoosePixelFormat has set above)
        if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
        	return(-2);

	/* Initialize Vulkan Instance */
	// 1. Fill VkApplicationInfo with app details
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// 2. Fill VkInstanceCreateInfo
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// 3. Create Vulkan instance
	
	vkCreateInstance(&createInfo, nullptr, &instance);

	/* Create Vulkan surface for Win32 */
	// 1. Fill VkWin32SurfaceCreateInfoKHR with HWND and HINSTANCE
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd = ghwnd;
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	// 2. Create the surface
	
	vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);

	/* Select a Physical Device (GPU) */
	// 1. Query number of GPUs
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);

	// 2. Get the first GPU handle
	VkPhysicalDevice physicalDevice;
	vkEnumeratePhysicalDevices(instance, &gpuCount, &physicalDevice);

	/*  Find Graphics and Present Queue Families */
	// 1. Query queue family properties
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	// 2. Find a queue family that supports graphics and present
	int graphicsFamily = -1, presentFamily = -1;
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			graphicsFamily = i;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
		if (presentSupport)
			presentFamily = i;
	}

	/* Create Logical Device and Queues */
	// 1. Specify queue creation info
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsFamily;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// 2. Fill VkDeviceCreateInfo
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	// 3. Create logical device
	
	vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

	// 4. Get queue handles
	VkQueue graphicsQueue, presentQueue;
	vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);

	/* Create a Swapchain */
	// 1. Query surface capabilities and formats
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

	// 2. Choose a format and present mode
	VkSurfaceFormatKHR surfaceFormat = formats[0];
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

	// 3. Fill VkSwapchainCreateInfoKHR
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;

	// 4. Create swapchain
	
	vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

	/* Create Image Views for Swapchain Images */
	// 1. Get swapchain images
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	std::vector<VkImage> swapchainImages(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

	// 2. Create image views
	std::vector<VkImageView> swapchainImageViews(imageCount);
	for (size_t i = 0; i < imageCount; i++) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceFormat.format;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]);
	}

	/* Create Render Pass */
	// 1. Define color attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// 2. Reference the attachment from a subpass
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// 3. Create subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// 4. Create render pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	
	vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	
	/* Create Framebuffers */
	std::vector<VkFramebuffer> swapchainFramebuffers(imageCount);
	for (size_t i = 0; i < imageCount; i++) {
		VkImageView attachments[] = { swapchainImageViews[i] };
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = surfaceCapabilities.currentExtent.width;
		framebufferInfo.height = surfaceCapabilities.currentExtent.height;
		framebufferInfo.layers = 1;
		vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]);
	}


	/* Create Command Pool and Buffers */
	// 1. Create command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamily;
	
	vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

	// 2. Allocate command buffers
	std::vector<VkCommandBuffer> commandBuffers(imageCount);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
	vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());

	/* Create Vertex Buffer for Triangle */
	// Define triangle vertices
	struct Vertex { float pos[2]; float color[3]; };
	Vertex vertices[] = {
		{{ 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
		{{ 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
		{{-0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }},
	};

	// Create VkBuffer and allocate VkDeviceMemory for vertex buffer (see Vulkan docs for details)


	/* Create Graphics Pipeline (Shaders, Pipeline Layout, etc.) */
	// 1. Load/compile SPIR-V shaders for vertex and fragment stages
	// 2. Create VkShaderModule for each shader
	// 3. Set up VkPipelineVertexInputStateCreateInfo, VkPipelineInputAssemblyStateCreateInfo, etc.
	// 4. Create VkPipelineLayout
	// 5. Create VkGraphicsPipeline
	// (See Vulkan tutorial for full details)


	/* Record Command Buffers */
	// For each framebuffer, begin command buffer, begin render pass, bind pipeline, bind vertex buffer, draw, end render pass, end command buffer
	for (size_t i = 0; i < imageCount; i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = surfaceCapabilities.currentExtent;

		VkClearValue clearColor = {};
		clearColor.color.float32[0] = 0.0f;
		clearColor.color.float32[1] = 0.0f;
		clearColor.color.float32[2] = 0.0f;
		clearColor.color.float32[3] = 1.0f;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		// Bind pipeline, vertex buffer, etc.
		
		vkCmdEndRenderPass(commandBuffers[i]);
		vkEndCommandBuffer(commandBuffers[i]);
	}
	return(0);
}

void resize(int width, int height)
{
	// code
	if (height == 0)
		height = 1;	// to avoid divided 0(Illeegal Instruction) in future cause

	// glViewport(0, 0, width, height); // screen cha konta bhag dakhvu... sadhya sagli screen dakhvli jat ahe pn he game need nusar customize karta yet.. eth width ai height he GL_SIZEI ya type che ahet ...resize la yetana te int ya type madhe yetat
}

void display(void) 
{
	// code

	/* Main Loop: Acquire, Submit, Present */
	// 1. Acquire next image from swapchain
	// 2. Submit command buffer to graphics queue
	// 3. Present image to surface

	SwapBuffers(ghdc); // 

}

void update(void)
{
	// code
}

void uninitialize(void)
{
	// function declarations
	void ToggleFullScreen(void); 

	// code
	if (gbFullScreen)
	{
		ToggleFullScreen();
	}

	/* Cleanup */
	// 1. Destroy Vulkan objects in reverse order of creation
	if (ghrc) 
	{
		//  rc); // OpenGL context ghdc la deun takaycha
		ghrc = NULL;
	}

	if (device) 
	{
		vkDestroyDevice(device, nullptr); // destroy logical device
		device = VK_NULL_HANDLE;
	}
	if (swapchain) 
	{
		vkDestroySwapchainKHR(device, swapchain, nullptr); // destroy swapchain
		swapchain = VK_NULL_HANDLE;
	}
	if (renderPass) 
	{
		vkDestroyRenderPass(device, renderPass, nullptr); // destroy render pass
		renderPass = VK_NULL_HANDLE;
	}
	if (commandPool) 
	{
		vkDestroyCommandPool(device, commandPool, nullptr); // destroy command pool
		commandPool = VK_NULL_HANDLE;
	}
	if (surface) 
	{
		vkDestroySurfaceKHR(instance, surface, nullptr); // destroy surface
		surface = VK_NULL_HANDLE;
	}
	if (instance) 
	{
		vkDestroyInstance(instance, nullptr); // destroy Vulkan instance
		instance = VK_NULL_HANDLE;
	}

	// destroy swapchain image views
	// for (auto& imageView : swapchainImageViews) 
	// {
	// 	vkDestroyImageView(device, imageView, nullptr);
	// }
	// destroy swapchain framebuffers
	// for (auto& framebuffer : swapchainFramebuffers) 
	// {
	// 	vkDestroyFramebuffer(device, framebuffer, nullptr);
	// }
	// destroy swapchain images
	// for (auto& image : swapchainImages) 
	// {
	// 	vkDestroyImage(device, image, nullptr);
	// }
	
	// destroy vertex buffer
	// vkDestroyBuffer(device, vertexBuffer, nullptr);
	// destroy vertex buffer memory
	// vkFreeMemory(device, vertexBufferMemory, nullptr);
	// destroy shader modules
	// vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	// vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

	// release DC


	if (ghdc) 
	{
		ReleaseDC(ghwnd, ghdc); // ata Win32 cha ghdc pn lagat nahi mhnun to ghwnd la deun takaycha and NULL karaycha
		ghdc = NULL;
	}

	if (ghwnd)
	{
		DestroyWindow(ghwnd); // sagl window vr draw karun zal mhnun ata ghwnd pn null karun takaycha
		ghwnd = NULL;
	}

	if (gpFile)
        {
        	fprintf(gpFile, "Log file Successfully Closed.\n");
        	fclose(gpFile);
        	gpFile = NULL;
        }

}


