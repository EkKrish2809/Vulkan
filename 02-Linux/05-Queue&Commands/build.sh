rm VulkanApp

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi  -o VulkanApp

./VulkanApp