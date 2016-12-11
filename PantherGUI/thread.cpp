
#include "thread.h"
#include <thread>

struct PGThread {
	std::thread thread;

	PGThread(PGThreadFunction function) : thread(function) { }
};


PGThreadHandle CreateThread(PGThreadFunction function) {
	PGThreadHandle handle = new PGThread(function);
	return handle;
}

void JoinThread(PGThreadHandle handle) {
	handle->thread.join();
}

void DetachThread(PGThreadHandle handle) {
	handle->thread.detach();
}

void DeleteThread(PGThreadHandle handle){ 
	delete handle;
}
