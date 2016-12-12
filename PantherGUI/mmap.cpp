
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

struct PGFile {
	FILE *f;
};

namespace mmap {
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

	PGFileHandle OpenFile(std::string filename, PGFileAccess access) {
		PGFileHandle handle = new PGFile();
		fopen_s(&handle->f, filename.c_str(), access == PGFileReadOnly ? "rb" : (access == PGFileReadWrite ? "wb" : "wb+"));
		if (!handle->f) {
			delete handle;
			return nullptr;
		}
		return handle;
	}

	void CloseFile(PGFileHandle handle) {
		fclose(handle->f);
	}

	void* ReadFile(std::string filename) {
		PGFileHandle handle = OpenFile(filename, PGFileReadOnly);
		if (!handle) {
			return nullptr;
		}

		FILE* f = handle->f;
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* string = (char*)malloc(fsize + 1);
		if (!string) {
			return nullptr;
		}
		fread(string, fsize, 1, f);
		CloseFile(handle);

		string[fsize] = 0;
		return string;
	}

	void WriteToFile(PGFileHandle handle, const char* text, ssize_t length) {
		if (length == 0) return;
		fwrite(text, sizeof(char), length, handle->f);
	}	
	
	void Flush(PGFileHandle handle) {
		fflush(handle->f);
	}


	void DestroyFileContents(void* address) {
		free(address);
	}
}