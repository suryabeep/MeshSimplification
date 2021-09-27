CFLAGS = -std=c++17
LDFLAGS = -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lcurand

.PHONY: main

all: main

main:
	nvcc $(CFLAGS) convolution.cu main.cpp glad.c -o main $(LDFLAGS)

.PHONY: clean

clean:
	rm -f *.o main