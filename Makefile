
export CC = gcc
export CFLAGS = -O3

.PHONY: main clean install

main:
	g++ -O3 -msse2 main.cpp Octree.cpp Pool.cpp Timer.cpp -o main

clean:
	rm -f *.o main

install:
	strip main
