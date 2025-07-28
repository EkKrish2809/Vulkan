rm vkx11_test

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi  -o vkx11_test

./vkx11_test