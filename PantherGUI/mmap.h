#pragma once

#include <string>

struct PGMemoryMappedFile;
typedef struct PGMemoryMappedFile* PGMemoryMappedFileHandle;

PGMemoryMappedFileHandle MemoryMapFile(std::string filename);
void* OpenMemoryMappedFile(PGMemoryMappedFileHandle);
void CloseMemoryMappedFile(void* address);
void DestroyMemoryMappedFile(PGMemoryMappedFileHandle handle);
void FlushMemoryMappedFile(void *address);