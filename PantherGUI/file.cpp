
#include "mmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

struct PGFile {
	FILE *f;
};

namespace PGmmap {
	PGFileHandle OpenFile(std::string filename, PGFileAccess access) {
		PGFileHandle handle = new PGFile();
#ifdef WIN32
		fopen_s(&handle->f, filename.c_str(), access == PGFileReadOnly ? "rb" : (access == PGFileReadWrite ? "wb" : "wb+"));
#else
		handle->f = fopen(filename.c_str(), access == PGFileReadOnly ? "rb" : (access == PGFileReadWrite ? "wb" : "wb+"));
#endif
		if (!handle->f) {
			delete handle;
			return nullptr;
		}
		return handle;
	}

	void CloseFile(PGFileHandle handle) {
		fclose(handle->f);
	}

	void* ReadFile(std::string filename, lng& result_size) {
		result_size = -1;
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
		result_size = (lng)fsize;

		string[fsize] = 0;
		return string;
	}

	void WriteToFile(PGFileHandle handle, const char* text, lng length) {
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