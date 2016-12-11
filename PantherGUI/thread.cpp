
#include "thread.h"
#include <thread>
#include <mutex>

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

void DestroyThread(PGThreadHandle handle){ 
	delete handle;
}

struct PGMutex {
	std::mutex mutex;
};

PGMutexHandle CreateMutex() {
	return new PGMutex();
}

void LockMutex(PGMutexHandle handle) {
	handle->mutex.lock();
}

void UnlockMutex(PGMutexHandle handle) {
	handle->mutex.unlock();
}

void DestroyMutex(PGMutexHandle handle) {
	delete handle;
}
