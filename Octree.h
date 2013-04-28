
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


//dummys
struct Vec
{
	float data[4];
	Vec(float x, float y, float z, float w = 0.0)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}
	
	Vec(const Vec& v)
	{
		data[0] = v.data[0];
		data[1] = v.data[1];
		data[2] = v.data[2];
		data[3] = v.data[3];
	}
	
	bool operator==(const Vec& v) const
	{
		return data[0] == v.data[0] &&
			data[1] == v.data[1] &&
			data[2] == v.data[2] &&
			data[3] == v.data[3];
	}
	
	float operator[](int i) const
	{
		return data[i];
	}
};


struct Vec3
{
	float data[3];
	Vec3(float x, float y, float z)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
	}
	
	Vec3(const Vec3& v)
	{
		data[0] = v.data[0];
		data[1] = v.data[1];
		data[2] = v.data[2];
	}
	
	bool operator==(const Vec3& v) const
	{
		return data[0] == v.data[0] &&
			data[1] == v.data[1] &&
			data[2] == v.data[2];
	}
	
	float operator[](int i) const
	{
		return data[i];
	}
};

inline bool isPow2(uint i)
{
	return ((i - 1) & i) == 0;
}

inline uint log2(uint n)
{
	assert(n != 0);
	assert(isPow2(n));
	
	uint log = 0;
	while(true)
	{
		n >>= 1;
		if(n == 0)
			break;
		log++;
	}
	return log;
}

//set bits in mask to 1 or 0 if i has bit[1,2,3] set
//#define set_if_bit1(value, i, mask) ((i&1) ? ((value) | (mask)) : ((value) & ~(mask)))
//#define set_if_bit2(value, i, mask) ((i&2) ? ((value) | (mask)) : ((value) & ~(mask)))
//#define set_if_bit3(value, i, mask) ((i&4) ? ((value) | (mask)) : ((value) & ~(mask)))

//set bits in mask to 1 (not 0, no toggle) if i has bit [1,2,3] set
#define set1_if_bit1(value, i, mask) ((i&1) ? ((value) | (mask)) : (value))
#define set1_if_bit2(value, i, mask) ((i&2) ? ((value) | (mask)) : (value))
#define set1_if_bit3(value, i, mask) ((i&4) ? ((value) | (mask)) : (value))

//some optimizations, but they seem to be slower for sparse Octrees
//#define squared_distance_search
//#define boundbox_merge

template<typename T>
class Octree
{
	public:

	class Branch;
	class Leaf;

	class leaf_iterator;

	class Node {};
	
	//Branch must have at least one child.
	class Branch : public Node
	{
		Node *children[8];

		public:

		inline Branch()
		{
			children[0] = 0; children[1] = 0; children[2] = 0; children[3] = 0;
			children[4] = 0; children[5] = 0; children[6] = 0; children[7] = 0;
		}

		inline void *operator new(size_t num_bytes, Pool<Branch> *mem) {
			assert(sizeof(Branch) == num_bytes);
			return mem->alloc_item();
		}

		inline Node* operator[](uint i) const {
			assert(i < 8);
			return children[i];
		}

		friend class Octree<T>;
	};

	class Leaf : public Node
	{
		T m_value;

		public:

		inline Leaf(T value) :
			m_value(value) {
		}

		inline void *operator new(size_t num_bytes, Pool<Leaf> *mem) {
			assert(sizeof(Leaf) == num_bytes);
			return mem->alloc_item();
		}

		inline T& value() {
			return m_value;
		}

		inline void value(T& v) {
			m_value = v;
		}

		friend class Octree<T>;
	};

	/*
	* A Stack to cache information while iterating over the OCtree.
	*/
	struct Stack
	{
		struct State
		{
			Branch* parent;
			int index;
		};

		int max_depth; //the depth where the leaves are
		int state_idx;

		State state[(sizeof(uint) * 8)];

		Stack(Branch* b, uint depth) :
			max_depth(depth),
			state_idx(-1) {

			assert(depth < (sizeof(uint) * 8));
			
			push(b, 0);
		}

		//0 is where the leaves are
		inline uint level() {
			return max_depth - state_idx;
		}

		inline void pop() {
			--state_idx;
		}

		inline int index() const {
			return state[state_idx].index;
		}

		inline Branch* parent() {
			return state[state_idx].parent;
		}

		inline void set(int index) {
			state[state_idx].index = index;
		}

		inline void push(Branch* parent, int index) {
			++state_idx;
			//  std::cout << "push: depth: " << parent->depth() << ", state_idx: " << state_idx << ", index: " << index << std::endl;
		   
			//assert((int) parent->depth() + state_idx == max_depth);
			//  assert((int) parent->depth() == level());
			
			state[state_idx].parent = parent;
			state[state_idx].index = index;
		}
	};
	
	class leaf_iterator : public std::iterator<std::input_iterator_tag, Leaf>
	{
		public:

		Stack* stack;

		inline leaf_iterator(Branch* b = NULL, uint depth = 0) {
			//std::cout << "init: depth: " << depth << std::endl;
			//std::cout << "init: b: " << ((ulong) b) << std::endl;
			
			if(b) {
				stack = new Stack(b, depth);
				
				down();
			} else {
				stack = 0;
			}
			//std::cout << "init done" << std::endl;
		}
		
		~leaf_iterator() {
			delete stack;
		}
		
		Leaf* leaf() const {
			if(stack == 0)
				return 0;
			
			const Branch* b = stack->parent();
			const uint i = stack->index();
			return reinterpret_cast<Leaf*>(b->children[i]);
		}
		
		inline leaf_iterator(const leaf_iterator& it) {
			stack = it.stack;
		}
		
		//move up the stack
		void up() {
			//std::cout << "up" << std::endl;
			assert(stack->state_idx != 0);
			stack->pop();
			
			//fill stack
			while(stack->state_idx >= 0)
			{
				Branch* b = stack->parent();
				uint i = stack->index() + 1;

				for(; i < 8; ++i) {
					Node* child = b->children[i];
					if(child == NULL)
						continue;
					
					//stack->top().index = i;
					stack->set(i);
					
					return;
				}
				stack->pop();
			}

			delete stack;
			stack = 0;
		}
		
		//move down the stack
		void down() {
			//std::cout << "down" << std::endl;

			Branch* b = stack->parent();
			uint i = stack->index();
			stack->pop();
			Node* n = 0;
			
			while(stack->state_idx < stack->max_depth)
			{
				for(; i < 8; ++i)
				{
					n = b->children[i];
					
					if(n == NULL)
						continue;
					
					stack->push(b, i);
					break;
				}
				//std::cout << "i: " << i << std::endl;
				
				if(stack->state_idx + 1 == stack->max_depth)
					break;
				
				b = reinterpret_cast<Branch*>(n);
				i = 0;
			}
		}
		
		leaf_iterator& operator++() {
		   // std::cout << "operator++" << std::endl;

			if(stack == 0)
				return *this;

			//assert(stack->top().parent->depth() == 1);

			Branch* b = stack->parent();
			uint i = stack->index() + 1;

			for(; i < 8; ++i)
			{
				Node* child = b->children[i];
				if(child == NULL)
					continue;

				//stack->top().index = i;
				stack->set(i);

				return *this;
			}

			up();
			if(stack)
				down();

			return *this;
		}
		
		//Vector position of the leaf.
		Vec position(float w = 0.0) const {
			uint x = 0;
			uint y = 0;
			uint z = 0;
			const uint max_depth = stack->max_depth;
			uint size = (1 << (max_depth-1));

			for(uint i = 0; i < max_depth; ++i)
			{
				const uint index = stack->state[i].index;
				
				x = set1_if_bit1(x, index, size);
				y = set1_if_bit2(y, index, size);
				z = set1_if_bit3(z, index, size);
				
				size >>= 1;
			}
			
			return Vec(x, y, z, w);
		}

		inline leaf_iterator operator++(int) {
			leaf_iterator tmp(*this);
			operator++();
			return tmp;
		}

		inline bool operator==(const leaf_iterator& rhs) {
			return leaf() == rhs.leaf();
		}

		inline bool operator!=(const leaf_iterator& rhs) {
			return leaf() != rhs.leaf();
		}

		inline Leaf& operator*() {
			return *leaf();
		}
	};

	typedef typename Pool<Leaf>::iterator leaves_iterator;
	typedef typename Pool<Branch>::iterator branches_iterator;

	Node* m_root;
	uint m_depth;
	uint leaf_count;
	uint branch_count;

	Pool<Leaf> leaf_pool;
	Pool<Branch> branch_pool;

//public:
	Octree(uint size) :
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
	inline uint width() const {
		return (1 << depth());
	}

	inline uint depth() const {
		return m_depth;
	}
	
	/*
	* Get number of possible nodes.
	*/
	inline ulong capacity() const
	{
		ulong w = width();
		return w * w * w;
	}
	
	/*
	* Get value at given position if Leaf exists.
	* Otherwise return a null pointer.
	*/
	Leaf* at(const uint x, const uint y, const uint z) const {
		Node* n = m_root;
		uint size = width();

		assert(x <= size);
		assert(y <= size);
		assert(z <= size);

		while(size != 1 && n)
		{
			size /= 2;

			n = reinterpret_cast<Branch*>(n)->children [
				!!(x & size) * 1 + !!(y & size) * 2 + !!(z & size) * 4
			];
		}

		return reinterpret_cast<Leaf*>(n);
	}

	/*
	* Insert a new Leaf and initialize it using the given value.
	* If the Leaf exists, just return the leaf.
	*/
	Leaf* insert(const uint x, const uint y, const uint z, const uint &value) {
		assert(x < width());
		assert(y < width());
		assert(z < width());

		Node** n = &m_root;
		Node** parent = &m_root;
		uint i = 0;
		uint depth = m_depth;

		while(depth)
		{
			if(*n == NULL)
			{
				*n = new(&branch_pool) Branch();
				++branch_count;
				parent = n;
			}
			else
			{
				--depth;
				parent = n;
				
				//	The nth bit of x, y and z is encoded in the index.
				//	Since size is always a power of two, size has always
				//	only one bit set and it is used as bit mask to check the nth bit.
				
				const uint size = (1 << depth);
				//same as: i = ((x & size) ? 1 : 0) + ((y & size) ? 2 : 0) + ((z & size) ? 4 : 0)
				i = !!(x & size) * 1 + !!(y & size) * 2 + !!(z & size) * 4; 
				n = &reinterpret_cast<Branch*>(*n)->children[i];
			}
		}

		if(*n == NULL)
		{
			assert(depth == 0);
			*n = new(&leaf_pool) Leaf(value); //*parent, value, i, 0);
			++leaf_count;
		}

		return reinterpret_cast<Leaf*>(*n);
	}
	
	//Nearest Neighbor Search in the Octree using bounding boxes
	struct NNSearchFull
	{
		//search for nearest neighbour to this position
		const int pos_x;
		const int pos_y;
		const int pos_z;

		//search box volume
		int x_min, x_max;
		int y_min, y_max;
		int z_min, z_max;

		//nearest neighbour
		Leaf* nn_leaf;
		ulong nn_sq_distance;
		uint nn_x;
		uint nn_y;
		uint nn_z;

		inline NNSearchFull(uint x, uint y, uint z) :
			pos_x(x), pos_y(y), pos_z(z),
			x_min(0), x_max(std::numeric_limits<int>::max()),
			y_min(0), y_max(std::numeric_limits<int>::max()),
			z_min(0), z_max(std::numeric_limits<int>::max()),
			nn_leaf(0), nn_sq_distance(std::numeric_limits<uint>::max()),
			nn_x(0), nn_y(0), nn_z(0) {
		}

		//virtual void found_leaf(Leaf* const leaf, const float distance)
		//{
		//}

		//check_leaf if the leaf at position x/y/z is nearer then the current leaf
		inline void check_leaf(Leaf* leaf, int x, int y, int z) {
			//assert(leaf->position() == Vec(x, y, z, 0));
			
			const int dx = pos_x - x;
			const int dy = pos_y - y;
			const int dz = pos_z - z;
			const ulong sq_distance = ((ulong) (dx*dx)) + ((ulong) (dy*dy)) + ((ulong) (dz*dz));

			//++lc;

			if(sq_distance < nn_sq_distance)
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
		inline bool check_branch(const int x, const int y, const int z, const int w) const {
			//bc++;
			return !(x_max < x || x_min > x+w || y_max < y || y_min > y+w || z_max < z || z_min > z+w);
		}
	
		//on_path might speed up search in dense areas (when the point is likely to exist at that place)
		void search(const Branch* b, const uint x, const uint y, const uint z, const uint size) {
			assert(b != NULL);
			assert(isPow2(size));
			//assert(b->width() / 2 == size);

			//try the path to the destined postion first
			const uint start_i = !!(pos_x & size) * 1 + !!(pos_y & size) * 2 + !!(pos_z & size) * 4;
			//const Leaf* old_leaf = nn_leaf; //store current leaf to check if the bouding box has changed

			for(uint i = start_i; i < (start_i+8); ++i)
			{
				Node* n = b->children[i & 7]; //limit index to range [0-7]

				if(n == 0)
					continue;

				const uint child_x = set1_if_bit1(x, i, size);
				const uint child_y = set1_if_bit2(y, i, size);
				const uint child_z = set1_if_bit3(z, i, size);

				if(size == 1) //child
				{
					//assert(n->type() == LeafNode);
					check_leaf(reinterpret_cast<Leaf*>(n), child_x, child_y, child_z);
				}
				else if(check_branch(child_x, child_y, child_z, size))
				{
					//assert(n->type() == BranchNode);
					search(reinterpret_cast<Branch*>(n), child_x, child_y, child_z, size/2);
				}
			}
		}
	};

	/*
	struct SearchBase
	{
		//search for nearest neighbour to this position
		const int pos_x;
		const int pos_y;
		const int pos_z;
		
		inline SearchBase(uint x, uint y, uint z) :
			pos_x(x), pos_y(y), pos_z(z)
		{
		}

		virtual void check_leaf(Leaf* leaf, const int x, const int y, const int z) = 0;
		virtual bool check_branch(const int x, const int y, const int z, const int w) const = 0;
		virtual void update_volume(const ulong sq_distance) = 0;

		//on_path might speed up search in dense areas (when the point is likely to exist at that place)
		void search(const Branch* b, const uint x, const uint y, const uint z, const uint size) //, const bool on_path = true)
		{
			assert(b != NULL);
			assert(isPow2(size));
			//assert(b->width() / 2 == size);
			
			//try the path to the destined postion first
			const uint start_i = !!(pos_x & size) * 1 + !!(pos_y & size) * 2 + !!(pos_z & size) * 4;
			
			for(uint i = start_i; i < (start_i+8); ++i)
			{
				Node* n = b->children[i & 7]; //limit index to range [0-7]
				
				if(n == 0)
					continue;
				
				const uint child_x = set1_if_bit1(x, i, size);
				const uint child_y = set1_if_bit2(y, i, size);
				const uint child_z = set1_if_bit3(z, i, size);
				
				if(size == 1) //child
				{
					//assert(n->type() == LeafNode);
					check_leaf(reinterpret_cast<Leaf*>(n), child_x, child_y, child_z);
				}
				else if(direct_path || check_branch(child_x, child_y, child_z, size))
				{
				  //  assert(n->type() == BranchNode);
					search(reinterpret_cast<Branch*>(n), child_x, child_y, child_z, size/2); //, direct_path);
				}
			}
		}
	};
	
	struct BoundingBoxSearch : public SearchBase
	{
		//search box volume
		int x_min, x_max;
		int y_min, y_max;
		int z_min, z_max;
		
		inline BoundingBoxSearch(uint x, uint y, uint z) :
			SearchBase(x, y, z),
			x_min(0), x_max(std::numeric_limits<int>::max()),
			y_min(0), y_max(std::numeric_limits<int>::max()),
			z_min(0), z_max(std::numeric_limits<int>::max())
		{
		}

		void update_volume(const ulong sq_distance)
		{
			const int r = std::sqrt(sq_distance) + 1.0; //<=> ceil(distance)
			
			#ifdef boundbox_merge
			x_min = max(pos_x - r, x_min);
			x_max = min(pos_x + r, x_max);
			y_min = max(pos_y - r, y_min);
			y_max = min(pos_y + r, y_max);
			z_min = max(pos_z - r, z_min);
			z_max = min(pos_z + r, z_max);
			#else
			x_min = SearchBase::pos_x - r;
			x_max = SearchBase::pos_x + r;
			y_min = SearchBase::pos_y - r;
			y_max = SearchBase::pos_y + r;
			z_min = SearchBase::pos_z - r;
			z_max = SearchBase::pos_z + r;
			#endif
		}
		
		//check if any point of the position is in the search box
		inline bool check_branch(const int x, const int y, const int z, const int w) const
		{
			//++SearchBase::bc;
			return !(
				x_max < x || x_min > x+w ||
				y_max < y || y_min > y+w ||
				z_max < z || z_min > z+w
			);
		}
	};
	
	template<class B>
	struct NNSearch : public B
	{
		//nearest neighbour
		Leaf* nn_leaf;
		ulong nn_sq_distance;
		uint nn_x;
		uint nn_y;
		uint nn_z;
		
		inline NNSearch(uint x, uint y, uint z) :
			B(x, y, z),
			nn_leaf(0), nn_sq_distance(std::numeric_limits<uint>::max()),
			nn_x(0), nn_y(0), nn_z(0)
		{
		}
		
		//check_leaf if the leaf at position x/y/z is nearer then the current leaf
		inline void check_leaf(Leaf* leaf, const int x, const int y, const int z)
		{
			//++SearchBase::lc;
			//assert(leaf->position() == Vec(x, y, z, 0));
			
			const int dx = SearchBase::pos_x - x;
			const int dy = SearchBase::pos_y - y;
			const int dz = SearchBase::pos_z - z;
			const ulong sq_distance = ((ulong) (dx*dx)) + ((ulong) (dy*dy)) + ((ulong) (dz*dz));
			
			if(sq_distance < nn_sq_distance)
			{
				nn_leaf = leaf;
				nn_sq_distance = sq_distance;
				nn_x = x;
				nn_y = y;
				nn_z = z;
				
				B::update_volume(sq_distance);
			}
		}
	};
	*/
/*
	//TODO: test&complete
	template<class B>
	struct KNNSearch : public B
	{
		std::vector<Leaf*>& nn_leaves;
		std::vector<float>& nn_distances;
		uint nn_counter;
		
		inline KNNSearch(uint x, uint y, uint z, std::vector<Leaf*>& nn_leaves, std::vector<float>& nn_distances) :
			B(x, y, z),
			nn_leaves(nn_leaves), nn_distances(nn_distances),
			nn_counter(0)
		{
		}
		
		//TODO: update bounding box
		inline void check_leaf(Leaf* leaf, const int x, const int y, const int z)
		{
			//++SearchBase::lc;
			//assert(leaf->position() == Vec(x, y, z, 0));
			
			const int dx = SearchBase::pos_x - x;
			const int dy = SearchBase::pos_y - y;
			const int dz = SearchBase::pos_z - z;
			const ulong sq_distance = ((ulong) (dx*dx)) + ((ulong) (dy*dy)) + ((ulong) (dz*dz));
			const float distance = std::sqrt(sq_distance);
			
			if(nn_counter < nn_leaves.size())
			{
				nn_leaves[nn_counter] = leaf;
				nn_distances[nn_counter] = distance;
				++nn_counter;
			}
			else if(distance < nn_distances.back())
			{
				assert(nn_counter == nn_leaves.size());
				for(uint i = 0; i < nn_distances.size(); ++i)
				{
					if(nn_distances[i] < distance)
						continue;
					
					const uint mv = nn_leaves.size() - i;
					memmove(&nn_leaves[i+1], &nn_leaves[i], mv * sizeof(Leaf*));
					memmove(&nn_distances[i+1], &nn_distances[i], mv * sizeof(float));
					
					nn_leaves[i] = leaf;
					nn_distances[i] = distance;
					
					B::update_volume(sq_distance);
					break;
				}
			}
		}
	};
	
	//search for the k nearest neighbours to coordiantes x/y/z (incomplete)
	//TODO: test&complete
	uint searchKNN(uint x, uint y, uint z, std::vector<Leaf*>& nn_leaves, std::vector<float>& nn_distances) const
	{
		assert(x <= width());
		assert(y <= width());
		assert(z <= width());
		assert(nn_leaves.size() == nn_distances.size());
		
		if(root() == NULL || nn_leaves.empty())
			return 0;
		
		KNNSearch<BoundingBoxSearch> knns(x, y, z, nn_leaves, nn_distances);
		knns.search(root(), 0, 0, 0, width() / 2);
		
		//resize vectors if they were not filled?
		//uint max = std::min(nkns.nn_counter, nn_leaves.size());
		//nn_leaves.resize(max);
		//nn_distances.resize(max);
		
		return knns.nn_counter;
	}
*/
	inline Leaf* searchNN(const Vec &p, Vec* nn_vec = NULL) const {
		return searchNN(p[0], p[1], p[2], nn_vec);
	}

	//search for the nearest neighbour to coordiantes x/y/z
	Leaf* searchNN(uint x, uint y, uint z, Vec* nn_vec = NULL) const {
		assert(x <= width());
		assert(y <= width());
		assert(z <= width());

		if(root() == NULL)
			return NULL;

		NNSearchFull nns(x, y, z);

		//NNSearch<BoundingBoxSearch> nns(x, y, z);
		//NNSearch<BoundingSphereSearch> nns(x, y, z);
		nns.search(root(), 0, 0, 0, width() / 2);

		//lcc += nns.lc;
		//bcc += nns.bc;

		if(nn_vec)
			*nn_vec = Vec(nns.nn_x, nns.nn_y, nns.nn_z, 0);

		return nns.nn_leaf;
	}
	
	typedef void (*Func)(uint x, uint y, uint z, T& value);
	
	void traverse(Func func) {
		if(m_root)
			traverse(m_root, width(), 0, 0, 0, func);
	}

	void traverse(Node* n, uint m, const uint x, const uint y, const uint z, Func func) {
		assert(n != NULL);
		assert(m != 0);

		if(m == 1)
		{
			//assert(n->type() == LeafNode);
			(*func)(x, y, z, reinterpret_cast<Leaf*>(n)->m_value);
		}
		else
		{
			//assert(n->type() == BranchNode);

			m >>= 1;
			Node* tmp;
			for(uint i = 0; i < 8; i++)
			{
				tmp = reinterpret_cast<Branch*>(n)->children[i];
				if(tmp == NULL)
					continue;
				
				traverse(tmp, m, set1_if_bit1(x, i, m), set1_if_bit2(y, i, m), set1_if_bit3(z, i, m), func);
			}
		}
	}
	
	inline Branch* root() const {
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
	* Iterate leaves flollowing the tree structure.
	* and provide the position.
	*/
	leaf_iterator leaf_begin2() {
		return leaf_iterator(root(), depth());
	}

	leaf_iterator leaf_end2() {
		return leaf_iterator(0, 0);
	}

	/*
	* Iterate over all Branch elements.
	* Uses the allocatesd memory chunks.
	*/
	inline branches_iterator branch_begin() {
		return branch_pool.begin();
	}

	inline branches_iterator branch_end() {
		return branch_pool.end();
	}

	uint countLeaves() {
		return leaf_count;
	}
	
	uint countBranches() {
		return branch_count;
	}
};

#endif
