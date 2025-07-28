// x11 libraries: The project uses the X11 libraries for window management and input handling.
#include <X11/Xlib.h> 
#include <X11/Xutil.h> // XVisualInfo
#include <X11/XKBlib.h> // for KeyBoard

// vulkan related headers
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#include <GL/glx.h>

// #include <vulkan/vulkan_xlib.h> // for Xlib surface creation // already included in vulkan.h
// #include <vulkan/vulkan_core.h> // for Vulkan core functions // already included in vulkan.h
// #include <vulkan/vulkan_xlib_xrandr.h> // for Xlib RandR extension


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
Bool fullScreen = False;

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
GLXFBConfig glxFBConfig;


int main()
{
    // function declarations
    void toggleFullscreen(void);

    GLXFBConfig *glxFBConfigs = NULL;
	GLXFBConfig bestGLXFBConfig;
	XVisualInfo *tmpXVisualInfo = NULL;
	int numFBConfig;

    XSetWindowAttributes windowAttribute;
	int styleMask;
	Atom wm_delete_window_atom;
	XEvent event;
	KeySym keysym;
	int screenWidth;
	int screenHeight;
	char keys[26];

    int frameBufferAttributes[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        // GLX_SAMPLE_BUFFERS, 1,
        // GLX_SAMPLES, 4,
        None
    };

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance\n";
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

	// query the number of available extensions
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    
    std::cout << extensionCount << " extensions supported\n";

	// create a VkExtensionProperties vector to hold the number of extensions queried above and print them
	std::vector<VkExtensionProperties> supported_extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supported_extensions.data());
	for (const auto& ext : supported_extensions) {
		std::cout << "Extension: " << ext.extensionName << "\n";
	}

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    // Main loop
    while (true) {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == Expose) {
            // Handle expose event
        }
        if (event.type == KeyPress) {
            break; // Exit on key press
        }
        if (event.type == ClientMessage) { // ClientMessage
            if (event.xclient.data.l[0] == wm_delete_window_atom) {
                break; // Exit on window close
            }
        }
    }

    // Cleanup
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
	event.xclient.data.l[0] = fullScreen ? 0 : 1;
	event.xclient.data.l[1] = wm_fullscreen_state_atom;

	// like sent and post messages in Windows OS
	XSendEvent(display,
			RootWindow(display, visualInfo->screen),
			False, // if this msg is for this window or for its child window
			SubstructureNotifyMask,
			&event);
}


bool initialize()
{
    


    return true; // Placeholder for initialization logic
}