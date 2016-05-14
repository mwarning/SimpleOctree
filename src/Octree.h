
#ifndef OCTREE_H
#define OCTREE_H

#include <iostream>
#include <assert.h>
#include <vector>
#include <limits>
#include <stdlib.h>
#include <cmath> //for sqrt

#include "Pool.h"

/*
* This is an Octree implementation for quick point
* insertion, retrival and nearest neighbour search.
* There is no support for removing a Leaf or Branch;
* but that makes the code easier and faster.
*/


inline bool isPow2(unsigned i) {
	return ((i - 1) & i) == 0;
}

inline unsigned log2(unsigned n) {
	assert(n != 0);
	assert(isPow2(n));

	unsigned log = 0;
	while(true)
	{
		n >>= 1;
		if (n == 0) {
			break;
		}
		log++;
	}
	return log;
}


//set bits in mask to 1 (not 0, no toggle) if i has bit [1,2,3] set
#define set1_if_bit1(value, i, mask) ((i&1) ? ((value) | (mask)) : (value))
#define set1_if_bit2(value, i, mask) ((i&2) ? ((value) | (mask)) : (value))
#define set1_if_bit3(value, i, mask) ((i&4) ? ((value) | (mask)) : (value))


template<typename T>
class Octree
{
public:

	class Node {};

	//Branch must have at least one child.
	class Branch : public Node
	{
		Node *children[8];

		public:

		Branch()
		{
			memset(children, 0, sizeof(children));
		}

		void *operator new(size_t num_bytes, Pool<Branch> *mem) {
			assert(sizeof(Branch) == num_bytes);
			return mem->alloc_item();
		}

		Node* operator[](unsigned i) const {
			assert(i < 8);
			return children[i];
		}

		friend class Octree<T>;
	};

	class Leaf : public Node
	{
		T m_value;

		public:

		Leaf(T value) : m_value(value) {
		}

		void *operator new(size_t num_bytes, Pool<Leaf> *mem) {
			assert(sizeof(Leaf) == num_bytes);
			return mem->alloc_item();
		}

		T& value() {
			return m_value;
		}

		void value(T& v) {
			m_value = v;
		}

		friend class Octree<T>;
	};


	typedef typename Pool<Leaf>::iterator leaves_iterator;
	typedef typename Pool<Branch>::iterator branches_iterator;

	Octree(unsigned size) :
		m_root(0), m_depth(log2(size)), 
		leaf_count(0), branch_count(0) {
		assert(isPow2(size));
		assert(size > 2);
	}

	~Octree() {
	}

	/*
	* Size of the bounding box.
	* Always a power of two.
	*/
	unsigned width() const {
		return (1 << depth());
	}

	/*
	* Maximum depth of the tree.
	*/
	unsigned depth() const {
		return m_depth;
	}

	/*
	* Maximum number of leaves.
	*/
	unsigned capacity() const {
		auto w = width();
		return w * w * w;
	}

	/*
	* Get value at given position if Leaf exists.
	* Otherwise return a null pointer.
	*/
	Leaf* at(const unsigned x, const unsigned y, const unsigned z) const {
		Node* n = m_root;
		unsigned size = width();

		assert(x <= size);
		assert(y <= size);
		assert(z <= size);

		while(size != 1 && n)
		{
			size /= 2;

			n = reinterpret_cast<Branch*>(n)->children[
				!!(x & size) * 1 + !!(y & size) * 2 + !!(z & size) * 4
			];
		}

		return reinterpret_cast<Leaf*>(n);
	}

	/*
	* Insert a new Leaf and initialize it using the given value.
	* If the Leaf exists, just return the leaf.
	*/
	Leaf* insert(const unsigned x, const unsigned y, const unsigned z, const T &value) {
		assert(x < width());
		assert(y < width());
		assert(z < width());

		Node** n = &m_root;
		Node** parent = &m_root;
		unsigned i = 0;
		unsigned depth = m_depth;

		while (depth) {
			if(*n == nullptr)
			{
				*n = new(&branch_pool) Branch();
				++branch_count;
				parent = n;
			}
			else
			{
				--depth;
				parent = n;

				// The nth bit of x, y and z is encoded in the index.
				// Since size is always a power of two, size has always
				// only one bit set and it is used as bit mask to check the nth bit.

				const unsigned size = (1 << depth);
				// Same as: i = ((x & size) ? 1 : 0) + ((y & size) ? 2 : 0) + ((z & size) ? 4 : 0);
				i = !!(x & size) * 1 + !!(y & size) * 2 + !!(z & size) * 4; 
				n = &reinterpret_cast<Branch*>(*n)->children[i];
			}
		}

		if(*n == nullptr)
		{
			assert(depth == 0);
			*n = new(&leaf_pool) Leaf(value);
			++leaf_count;
		}

		return reinterpret_cast<Leaf*>(*n);
	}

	//search for the nearest neighbour to coordiantes x/y/z
	Leaf* findNearestNeighbour(unsigned x, unsigned y, unsigned z) const {
		unsigned found_x;
		unsigned found_y;
		unsigned found_z;

		return findNearestNeighbour(x, y, z, found_x, found_y, found_z);
	}

	Leaf* findNearestNeighbour(unsigned x, unsigned y, unsigned z, unsigned &found_x, unsigned &found_y, unsigned &found_z) const {
		assert(x <= width());
		assert(y <= width());
		assert(z <= width());

		if (root() == nullptr) {
			return nullptr;
		}

		NearestNeighbourSearchFull nns(x, y, z);

		nns.search(root(), 0, 0, 0, width() / 2);

		found_x = nns.nn_x;
		found_y = nns.nn_y;
		found_z = nns.nn_z;

		return nns.nn_leaf;
	}

	typedef void (*Func)(unsigned x, unsigned y, unsigned z, T& value);

	void traverse(Func func) {
		if (m_root) {
			traverse(m_root, width(), 0, 0, 0, func);
		}
	}

	Branch* root() const {
		return reinterpret_cast<Branch*>(m_root);
	}

	/*
	* Iterate leaves flollowing memory chunks.
	* The order is not defined.
	*/
	leaves_iterator leaf_begin() {
		return leaf_pool.begin();
	}

	leaves_iterator leaf_end() {
		return leaf_pool.end();
	}

	/*
	* Iterate over all Branch elements.
	* Uses the allocated memory chunks.
	*/
	branches_iterator branch_begin() {
		return branch_pool.begin();
	}

	branches_iterator branch_end() {
		return branch_pool.end();
	}

	unsigned countLeaves() {
		return leaf_count;
	}

	unsigned countBranches() {
		return branch_count;
	}

private:

	void traverse(Node* n, unsigned m, const unsigned x, const unsigned y, const unsigned z, Func func) {
		assert(n != nullptr);
		assert(m != 0);

		if (m == 1) {
			(*func)(x, y, z, reinterpret_cast<Leaf*>(n)->m_value);
		} else {
			m >>= 1;
			Node* tmp;
			for (unsigned i = 0; i < 8; i++) {
				tmp = reinterpret_cast<Branch*>(n)->children[i];
				if (tmp == nullptr) {
					continue;
				}

				traverse(tmp, m, set1_if_bit1(x, i, m), set1_if_bit2(y, i, m), set1_if_bit3(z, i, m), func);
			}
		}
	}

	// Nearest Neighbor Search in the Octree using bounding boxes.
	struct NearestNeighbourSearchFull
	{
		// Search for nearest neighbour to this position.
		const int pos_x;
		const int pos_y;
		const int pos_z;

		//search box volume
		int x_min, x_max;
		int y_min, y_max;
		int z_min, z_max;

		//nearest neighbour
		Leaf* nn_leaf;
		unsigned long nn_sq_distance;
		unsigned nn_x;
		unsigned nn_y;
		unsigned nn_z;

		NearestNeighbourSearchFull(unsigned x, unsigned y, unsigned z) :
			pos_x(x), pos_y(y), pos_z(z),
			x_min(0), x_max(std::numeric_limits<int>::max()),
			y_min(0), y_max(std::numeric_limits<int>::max()),
			z_min(0), z_max(std::numeric_limits<int>::max()),
			nn_leaf(0), nn_sq_distance(std::numeric_limits<unsigned>::max()),
			nn_x(0), nn_y(0), nn_z(0) {
		}

		//check_leaf if the leaf at position x/y/z is nearer then the current leaf
		void check_leaf(Leaf* leaf, int x, int y, int z) {
			const long dx = pos_x - x;
			const long dy = pos_y - y;
			const long dz = pos_z - z;
			const unsigned sq_distance = (dx * dx) + (dy * dy) + (dz * dz);

			//++lc;

			if (sq_distance < nn_sq_distance)
			{
				nn_leaf = leaf;
				nn_sq_distance = sq_distance;
				nn_x = x;
				nn_y = y;
				nn_z = z;

				const int r = std::sqrt(sq_distance) + 1.0; //<=> ceil(distance)

				x_min = pos_x - r;
				x_max = pos_x + r;
				y_min = pos_y - r;
				y_max = pos_y + r;
				z_min = pos_z - r;
				z_max = pos_z + r;
			}
		}

		//check if any point of the position is in the search box
		bool check_branch(const unsigned x, const unsigned int y, const unsigned int z, const unsigned w) const {
			return !(x_max < x || x_min > x+w || y_max < y || y_min > y+w || z_max < z || z_min > z+w);
		}

		void search(const Branch* b, const unsigned x, const unsigned y, const unsigned z, const unsigned size) {
			assert(b != nullptr);
			assert(isPow2(size));

			//try the path to the destined postion first
			const unsigned start_i = !!(pos_x & size) * 1 + !!(pos_y & size) * 2 + !!(pos_z & size) * 4;

			for (unsigned i = start_i; i < (start_i + 8); ++i) {
				Node* n = b->children[i & 7]; //limit index to range [0-7]

				if (n == nullptr) {
					continue;
				}

				const unsigned child_x = set1_if_bit1(x, i, size);
				const unsigned child_y = set1_if_bit2(y, i, size);
				const unsigned child_z = set1_if_bit3(z, i, size);

				if (size == 1) //child
				{
					check_leaf(reinterpret_cast<Leaf*>(n), child_x, child_y, child_z);
				}
				else if (check_branch(child_x, child_y, child_z, size))
				{
					search(reinterpret_cast<Branch*>(n), child_x, child_y, child_z, (size / 2));
				}
			}
		}
	};

	Node* m_root;
	unsigned m_depth;
	unsigned leaf_count;
	unsigned branch_count;

	Pool<Leaf> leaf_pool;
	Pool<Branch> branch_pool;
};

#endif
