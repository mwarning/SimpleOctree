
#ifndef TIMER_H
#define TIMER_H


#include <sys/time.h>
#include <sys/resource.h>

/*
* A very simple timer.
*/
struct Timer
{
	//clock_t start_time;
	double start_time;

	static double get_time() {
		struct timezone tzp;
		struct timeval t;

		gettimeofday(&t, &tzp);
		return t.tv_sec + t.tv_usec*1e-6;
	}

	Timer(bool start_now = false) {
		start_time = start_now ? get_time() : 0;
	}
	
	void start() {
		start_time = get_time();
	}
	
	double stop() {
		return get_time() - start_time;
	}
};

#endif
