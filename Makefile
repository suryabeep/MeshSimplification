CFLAGS = -std=c++17
LDFLAGS = -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl

.PHONY: main

all: main

main:
	g++ $(CFLAGS) main.cpp glad.c -o main $(LDFLAGS)

.PHONY: clean

clean:
	rm -f *.o main