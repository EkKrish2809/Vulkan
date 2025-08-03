

# ================= AS FAR AS IT TAKES =================== #

# Created Xlib based VkInstance 
# fetched available extensions and printed them

UPDATE
# Added physical device selection based on Descret GPU and Geometry shader support


16/07/25 -> Need to exxplore the Memory & Resources chapter.. Lots of memory creation/allocation methodes but I couldn't use it as I'm not able to direct its use with clearity

# Swapchains
Swapchain is part of Vulkan extensions, because it is dependent on the platform/OS the vulkan app running on.
Also, most Vulkan apps don't need to present the images on screen, rather to save on disk or something else. (eg. compute application)
So use the extension specific to your platform and create SwapChain.

# CommandBuffers
Swapchain provides number of images it can have. So we should create the CommandBuffers for all the images.
i.e. Swapchain Images count == CommandBuffers count

<== Primary Command Buffer ==>
You can submit the Primary Command buffer in the Queue, but can not call it from another Primary Command Buffer

<== Secondary Command Buffers ==>
You can not Submit the Secondary Command Buffer in the Queue, but can call it from another Primary Command Buffer.
Eg. We can store some functionality in Secondary Command Buffer and can call it from many different 'Primary Command Buffer'


# Vulkan Memory Fragmentation
Memory fragmentation in Vulkan command buffers refers to a situation where the memory allocated for command buffers becomes scattered, leading to inefficient use of memory. 
This can occur due to frequent allocations and deallocations of command buffers, resulting in situations where there is enough free memory overall, but it is not in a continuous block, 
making it difficult to allocate new command buffers.


# Staging Buffers
We created a Vertex Buffer for sharing Vertex data to the GPU in previous step (check 09-Triangle), which worked and we have a triangle on screen. 
But the memory type that Vertex Buffer uses is not Optimal memory for GPU to read the data from. In order to use this Optimal Memory type (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), 
we have to use Staging buffers to carrying the work of copying the data.
