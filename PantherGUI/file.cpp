
#include "mmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <algorithm>

struct PGFile {
	FILE *f;
};

namespace panther {
	PGFileHandle OpenFile(std::string filename, PGFileAccess access, PGFileError& error) {
		PGFileHandle handle = new PGFile();
		error = PGFileSuccess;
#ifdef WIN32
		errno_t retval = fopen_s(&handle->f, filename.c_str(), access == PGFileReadOnly ? "rb" : (access == PGFileReadWrite ? "wb" : "wb+"));
		if (!handle->f) {
			switch(retval) {
			case ENOMEM:
			case ENOSPC:
				error = PGFileNoSpace;
				break;
			case EFBIG:
				error = PGFileTooLarge;
				break;
			case EBUSY:
			case EIO:
			case ENFILE:
				error = PGFileIOError;
				break;
			case EROFS:
				error = PGFileReadOnlyFS;
				break;
			case ENAMETOOLONG:
				error = PGFileNameTooLong;
				break;
			case EPERM:
			case EACCES:
				error = PGFileAccessDenied;
				break;
			default:
				break;
			}
		}
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

	void* ReadFile(std::string filename, lng& result_size, PGFileError& error) {
		result_size = -1;
		PGFileHandle handle = OpenFile(filename, PGFileReadOnly, error);
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

	void* ReadPreview(std::string filename, lng max_size, lng& result_size, PGFileError& error) {
		result_size = -1;
		PGFileHandle handle = OpenFile(filename, PGFileReadOnly, error);
		if (!handle) {
			return nullptr;
		}
		FILE* f = handle->f;

		fseek(f, 0, SEEK_END);
		lng fsize = (lng) ftell(f);
		fseek(f, 0, SEEK_SET);

		max_size = std::min(fsize, max_size);

		char* string = (char*)malloc(max_size + 1);
		if (!string) {
			return nullptr;
		}
		fread(string, max_size, 1, f);
		CloseFile(handle);

		string[max_size] = '\0';
		result_size = max_size;
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