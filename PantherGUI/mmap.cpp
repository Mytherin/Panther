
#include "mmap.h"

#include <windows.h>

struct PGMemoryMappedFile {
	HANDLE file;
	HANDLE mmap;
	PGMemoryMappedFile(HANDLE file, HANDLE mmap) : file(file), mmap(mmap) { }
};

PGMemoryMappedFileHandle MemoryMapFile(std::string filename) {
	HANDLE file = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file) {
		// FIXME: check error
		return NULL;
	}
	HANDLE mmap = CreateFileMapping(file, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!mmap) {
		// FIXME: check error
		return NULL;
	}
	return new PGMemoryMappedFile(file, mmap);
}

void* OpenMemoryMappedFile(PGMemoryMappedFileHandle mmap) {
	void *mmap_location = MapViewOfFile(mmap->mmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!mmap_location) {
		// FIXME: check error
		return NULL;
	}
	return mmap_location;
}

void CloseMemoryMappedFile(void* address) {
	UnmapViewOfFile(address);
}

void DestroyMemoryMappedFile(PGMemoryMappedFileHandle handle) {
	CloseHandle(handle->mmap);
	CloseHandle(handle->file);
}

void FlushMemoryMappedFile(void *address) {
	FlushViewOfFile(address, 0);
}