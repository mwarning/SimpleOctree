
#include <cstdlib>

#include "Octree.hpp"


void showNearestNeighbour(Octree<unsigned> &octree, unsigned x, unsigned y, unsigned z)
{
	std::printf("Search for nearest element to position (%u, %u, %u)\n", x, y, z);

	unsigned found_x;
	unsigned found_y;
	unsigned found_z;

	auto leaf = octree.findNearestNeighbour(x, y, z, found_x, found_y, found_z);
	if (leaf) {
		std::printf("  => Found nearest element at (%u, %u, %u) with value %u\n", found_x, found_y, found_z, leaf->value());
	} else {
		std::printf("  => No nearest element found - octree empty?\n");
	}
}

int main(int argc, char **argv)
{
	// Cube side span of 2048 units.
	unsigned width = 2048;

	// Create an Octree.
	Octree<unsigned> octree(width);

	// Seed random generator.
	std::srand(std::time(0));

	// Insert elements at random locations.
	for (unsigned i = 0; i < 1000; i++)
	{
		unsigned x = std::rand() % width;
		unsigned y = std::rand() % width;
		unsigned z = std::rand() % width;
		octree.insert(x, y, z, i);
	}

	std::printf("Branches: %u\n", octree.countBranches());
	std::printf("Leaves: %u\n", octree.countLeaves());

	showNearestNeighbour(octree, 444, 23, 1333);

	return 0;
}
