# g++ main.cpp -lX11 -lglfw -lvulkan -ldl -lpthread -lXrandr -lXi
g++ vkx11_main.cpp -g -lX11 -lvulkan -ldl -lpthread -lXrandr -lXi -lGL -lGLX -o vkx11_test