#pragma once

struct PGThread;
typedef struct PGThread* PGThreadHandle;

typedef void(*PGThreadFunction)(void);

PGThreadHandle CreateThread(PGThreadFunction function);
void JoinThread(PGThreadHandle);
void DetachThread(PGThreadHandle);
void DeleteThread(PGThreadHandle);