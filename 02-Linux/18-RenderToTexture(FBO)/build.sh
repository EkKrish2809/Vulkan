rm VulkanApp
rm shaders/bin/*.spv

glslc shaders/lightPV.vert -o shaders/bin/lightPV.vert.spv

glslc shaders/lightPV.frag -o shaders/bin/lightPV.frag.spv

glslc shaders/lightPF.vert -o shaders/bin/lightPF.vert.spv

glslc shaders/lightPF.frag -o shaders/bin/lightPF.frag.spv

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi  -o VulkanApp

./VulkanApp