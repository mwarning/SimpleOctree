#ifndef POOL_H
#define POOL_H

#include <iostream>
#include <assert.h>
#include <ctime>
#include <cstring>
#include <stdlib.h>

/*
* This is a simple block based allocator
* that allocates lists of memory blocks.
*/

template<typename T, int LEN = 1024>
struct Pool
{
	struct Chunk
	{
		T items[LEN];
		Chunk* next;
	};

	class iterator : public std::iterator<std::input_iterator_tag, T>
	{
		public:

		Chunk *chunk;
		unsigned pos;

		public:

		iterator() {
		}
	
		iterator(Chunk* chunk, unsigned pos)
			: chunk(chunk), pos(pos) {
		}

		~iterator()
		{}

		inline iterator(const iterator& it) : chunk(it.chunk), pos(it.pos) {}
		inline iterator& operator++() {
			++pos;
			if (pos == LEN && chunk->next) {
				pos = 0;
				chunk = chunk->next;
			}
			return *this;
		}

		inline iterator operator++(int) {
			iterator tmp(*this);
			operator++();
			return tmp;
		}

		inline bool operator==(const iterator& rhs) {
			return chunk == rhs.chunk && pos == rhs.pos;
		}

		inline bool operator!=(const iterator& rhs) {
			return !(chunk == rhs.chunk && pos == rhs.pos);
		}
		
		inline T& operator*() {
			return chunk->items[pos];
		}
		
		inline T& operator->() {
			return chunk->items[pos];
		}
	};
	
	Pool() : pos_m(0) {
		init_resources();
	}
	
	~Pool() {
		free_resources();
	}
	
	void clear() {
		free_resources();
		init_resources();
	}
	
	unsigned countElements() {
		unsigned count = 0;
		Chunk* cur = beg_m;
		while(cur != end_m) {
			cur = cur->next;
			count += LEN;
		}
		
		return count + pos_m;
	}

	iterator begin() {
		return iterator(beg_m, 0);
	}

	iterator end() {
		return iterator(end_m, pos_m);
	}

	void* alloc_item() {
		assert(pos_m <= LEN);

		if (pos_m == LEN) {
			Chunk* chunk = (Chunk*) malloc(sizeof(Chunk));
			assert(chunk != NULL);
			
			chunk->next = NULL;
			end_m->next = chunk;
			end_m = chunk;
			pos_m = 0;
		}
		
		T* mem = &end_m->items[pos_m];
		++pos_m;
		
		return mem;
	}

private:

	void init_resources() {
		beg_m = (Chunk*) malloc(sizeof(Chunk));
		beg_m->next = NULL;
		end_m = beg_m;
	}

	//free all resources
	void free_resources() {
		Chunk* cur = beg_m;
		while(cur != end_m)
		{
			Chunk* tmp = cur->next;
			free(cur);
			cur = tmp;
		}
	}

	Chunk* beg_m;
	Chunk* end_m;
	unsigned pos_m;
};

#endif