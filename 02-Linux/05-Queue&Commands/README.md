# Created Xlib based VkInstance 
# fetched available extensions and printed them

UPDATE
# Added physical device selection based on Descret GPU and Geometry shader support


16/07/25 -> Need to exxplore the Memory & Resources chapter.. Lots of memory creation/allocation methodes but I couldn't use it as I'm not able to direct its use with clearity


Swapchain is part of Vulkan extensions, because it is dependent on the platform/OS the vulkan app running on.
Also, most Vulkan apps don't need to present the images on screen, rather to save on disk or something else. (eg. compute application)
So use the extension specific to your platform and create SwapChain.