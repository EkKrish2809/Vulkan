rm VulkanApp

glslc shaders/tessellation.vert -o shaders/tessellation.vert.spv

glslc shaders/tessellation.frag -o shaders/tessellation.frag.spv

glslc shaders/tessellationControl.tesc -o shaders/tessellationControl.tesc.spv

glslc shaders/tessellationEval.tese -o shaders/tessellationEval.tese.spv

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi  -o VulkanApp

./VulkanApp