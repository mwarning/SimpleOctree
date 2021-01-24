
#include <iostream>

#include "Octree.hpp"


// pseudo random number generator 
unsigned rand(unsigned seed) {
	const unsigned a = 16807;
	const unsigned m = 2147483647;
	return ((a * seed) % m);
}

void test1() {
	// Cube side span of 2048 units.
	unsigned width = 2048;

	// Create an Octree.
	Octree<unsigned> octree(width);

	// Insert elements at random locations.
	for (unsigned i = 0; i < 1000; i++) {
		unsigned x = rand() % width;
		unsigned y = rand() % width;
		unsigned z = rand() % width;
		octree.insert(x, y, z, i);
	}

	//std::cout << "Branches: " << octree.countBranches() << std::endl;
	//std::cout << "Leaves: " << octree.countLeaves() << std::endl;

	// find leaf nearest to [444, 23, 1333]
	const unsigned find_x = 444;
	const unsigned find_y = 23;
	const unsigned find_z = 1333;

	unsigned found_x;
	unsigned found_y;
	unsigned found_z;

	auto leaf = octree.findNearestNeighbour(find_x, find_y, find_z, found_x, found_y, found_z);
	if (leaf && found_x == 204 && found_y == 469 && found_z == 1301 && leaf->value() == 452) {
		std::cout << "Test1 [success]" << std::endl;
	} else {
		std::cout << "Test1 [failed]" << std::endl;
	}
}

void test2() {
	Octree<unsigned> octree(8);
	octree.insert(0, 2, 3, 23);
	octree.insert(0, 4, 5, 42);

	unsigned found_x;
	unsigned found_y;
	unsigned found_z;
	auto leaf = octree.findNearestNeighbour(0, 2, 4, found_x, found_y, found_z);

	if (leaf && found_x == 0 && found_y == 2 && found_z == 3 && leaf->value() == 23) {
		std::cout << "Test2: [success]" << std::endl;
	} else {
		std::cout << "Test2: [failed]" << std::endl;
	}
}

int main(int argc, char **argv) {
	test1();
	test2();
	return 0;
}
