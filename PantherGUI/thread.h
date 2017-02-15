#pragma once

#include <thread>
#include <mutex>

typedef void(*PGThreadFunction)(void);

struct PGThread {
	std::thread thread;

	PGThread(PGThreadFunction function) : thread(function) {}
};

typedef struct PGThread* PGThreadHandle;

PGThreadHandle CreateThread(PGThreadFunction function);
void JoinThread(PGThreadHandle);
void DetachThread(PGThreadHandle);
void DestroyThread(PGThreadHandle);

struct PGMutex {
	std::mutex mutex;
};

typedef struct PGMutex* PGMutexHandle;

PGMutexHandle CreateMutex(void);
void LockMutex(PGMutexHandle);
void UnlockMutex(PGMutexHandle);