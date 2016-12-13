
#include "mmap.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

struct PGMemoryMappedFile {
	HANDLE file;
	HANDLE mmap;
	PGMemoryMappedFile(HANDLE file, HANDLE mmap) : file(file), mmap(mmap) {}
};

namespace PGmmap {
	PGMemoryMappedFileHandle MemoryMapFile(std::string filename) {
		HANDLE file = CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (!file) {
			// FIXME: check error
			return nullptr;
		}
		HANDLE mmap = CreateFileMapping(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if (!mmap) {
			// FIXME: check error
			return nullptr;
		}
		return new PGMemoryMappedFile(file, mmap);
	}

	void* OpenMemoryMappedFile(PGMemoryMappedFileHandle mmap) {
		void *mmap_location = MapViewOfFile(mmap->mmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (!mmap_location) {
			// FIXME: check error
			return nullptr;
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
}