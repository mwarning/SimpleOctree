
CFLAGS = -O3

.PHONY: main clean

main: clean
	$(CXX) $(CFLAGS) src/main.cpp src/Octree.cpp src/Pool.cpp -o main

clean:
	rm -f main
