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

typedef enum {
	PGFileSuccess,
	PGFileAccessDenied,
	PGFileNoSpace,
	PGFileTooLarge,
	PGFileReadOnlyFS,
	PGFileNameTooLong,
	PGFileIOError,
	PGFileEncodingFailure
} PGFileError;

namespace panther {
	PGMemoryMappedFileHandle MemoryMapFile(std::string filename);
	void* OpenMemoryMappedFile(PGMemoryMappedFileHandle);
	void CloseMemoryMappedFile(void* address);
	void DestroyMemoryMappedFile(PGMemoryMappedFileHandle handle);
	void FlushMemoryMappedFile(void *address);

	PGFileHandle OpenFile(std::string filename, PGFileAccess access, PGFileError& error);
	void CloseFile(PGFileHandle handle);
	void WriteToFile(PGFileHandle handle, const char* text, lng length);
	void Flush(PGFileHandle handle);
	void* ReadFile(std::string filename, lng& result_size, PGFileError& error);
	void* ReadPreview(std::string filename, lng max_size, lng& result_size, PGFileError& error);
	void DestroyFileContents(void* address);
}
