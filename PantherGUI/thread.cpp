
#include "thread.h"

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

PGMutexHandle CreateMutex() {
	return new PGMutex();
}

void LockMutex(PGMutexHandle handle) {
	handle->mutex.lock();
}

void UnlockMutex(PGMutexHandle handle) {
	handle->mutex.unlock();
}
