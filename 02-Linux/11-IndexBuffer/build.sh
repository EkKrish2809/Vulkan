rm VulkanApp

glslc shaders/triangle.vert -o shaders/triangle.vert.spv

glslc shaders/triangle.frag -o shaders/triangle.frag.spv

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi  -o VulkanApp

./VulkanApp