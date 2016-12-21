#pragma once

struct PGThread;
typedef struct PGThread* PGThreadHandle;

typedef void(*PGThreadFunction)(void);

PGThreadHandle CreateThread(PGThreadFunction function);
void JoinThread(PGThreadHandle);
void DetachThread(PGThreadHandle);
void DestroyThread(PGThreadHandle);

struct PGMutex;
typedef struct PGMutex* PGMutexHandle;

PGMutexHandle CreateMutex(void);
void LockMutex(PGMutexHandle);
void UnlockMutex(PGMutexHandle);
void DestroyMutex(PGMutexHandle);