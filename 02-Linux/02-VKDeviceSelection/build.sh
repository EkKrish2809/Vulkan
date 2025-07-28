# g++ main.cpp -lX11 -lglfw -lvulkan -ldl -lpthread -lXrandr -lXi
rm vkx11_test

g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi -lpthread  -o vkx11_test

./vkx11_test