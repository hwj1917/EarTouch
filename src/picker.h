#ifndef picker_h
#define picker_h

#include "frame.h"
#include <pthread.h>

class Lock {
public:
	volatile int status;
	Lock() {
		status = 1;
	}
	void lock() {
		while (!status) {
		}
		status = 0;
	}
	void unlock() {
		status = 1;
	}
};

class Picker {
public:
    static pthread_mutex_t frames_mutex;
    static deque<Frame> frames;
    static vector<long long> delays;
    
    static Frame frame_down;
    static Frame frame_stable;
    
    static void getLog();
};

#endif /* picker_h */
