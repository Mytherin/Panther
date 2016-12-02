#pragma once

#include <string>
#include "utils.h"

struct PGMemoryMappedFile;
typedef struct PGMemoryMappedFile* PGMemoryMappedFileHandle;

struct PGFile;
typedef struct PGFile* PGFileHandle;

typedef enum {
	PGFileReadOnly,
	PGFileReadWrite
} PGFileAccess;

namespace mmap {
	PGMemoryMappedFileHandle MemoryMapFile(std::string filename);
	void* OpenMemoryMappedFile(PGMemoryMappedFileHandle);
	void CloseMemoryMappedFile(void* address);
	void DestroyMemoryMappedFile(PGMemoryMappedFileHandle handle);
	void FlushMemoryMappedFile(void *address);

	PGFileHandle OpenFile(std::string filename, PGFileAccess access);
	void CloseFile(PGFileHandle handle);
	void WriteToFile(PGFileHandle handle, const char* text, ssize_t length);
	void Flush(PGFileHandle handle);
	void* ReadFile(std::string filename);
	void DestroyFileContents(void* address);
}
