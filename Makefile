
CC = gcc
CFLAGS = -O3
LFLAGS = -lSDL -lGL -lc -lstdc++ -lm

.PHONY: main clean

OBJS = build/main.o build/Octree.o build/Pool.o build/Timer.o

build/%.o : src/%.cpp src/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

main: clean $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o main

clean:
	rm -f build/*.o main
