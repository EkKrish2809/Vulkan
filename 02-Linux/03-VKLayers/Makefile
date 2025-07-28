CFLAGS = -std=c++17 -O2
LDFLAGS =  -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

vkx11_test: vkx11_main.cpp
    g++ $(CFLAGS) vkx11_main.cpp $(LDFLAGS) -o vkx11_test

.PHONY: test clean

test: vkx11_test
    ./vkx11_test

clean:
    rm -f vkx11_test