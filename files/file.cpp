
#include "mmap.h"
#include "replaymanager.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <algorithm>

struct PGRegularFile {
	FILE *f;
};

namespace panther {
	PGFileHandle OpenFile(std::string filename, PGFileAccess access, PGFileError& error) {
		PGFileHandle handle = new PGRegularFile();
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
				error = PGFileIOError;
				break;
			}
		}
#else
		handle->f = fopen(filename.c_str(), access == PGFileReadOnly ? "rb" : (access == PGFileReadWrite ? "wb" : "wb+"));
		if (!handle->f) {
			error = PGFileIOError;
		}
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

	size_t GetFileSize(PGFileHandle handle) {
		FILE* f = handle->f;
		if (fseek(f, 0, SEEK_END) != 0) {
			return (size_t)-1;
		}
		long fsize = ftell(f);
		if (fsize < 0) {
			return (size_t)-1;
		}
		if (fseek(f, 0, SEEK_SET) != 0) {
			return (size_t)-1;
		}
		return fsize;
	}

	size_t ReadFromFile(PGFileHandle handle, char* buffer, size_t buffer_size) {
		return fread(buffer, buffer_size, 1, handle->f);
	}

	void* ReadFile(PGFileHandle handle, lng& result_size, PGFileError& error) {
		error = PGFileSuccess;
		FILE* f = handle->f;
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* string = (char*)malloc(fsize + 1);
		if (!string) {
			error = PGFileIOError;
			return nullptr;
		}
		fread(string, fsize, 1, f);
		CloseFile(handle);
		result_size = (lng)fsize;

		string[fsize] = 0;
		return string;
	}

	void* ReadFile(std::string filename, lng& result_size, PGFileError& error) {
		error = PGFileSuccess;
		result_size = -1;
		if (PGGlobalReplayManager::running_replay) {
			return PGGlobalReplayManager::ReadFile(filename, result_size, error);
		}
		PGFileHandle handle = OpenFile(filename, PGFileReadOnly, error);
		if (!handle) {
			if (PGGlobalReplayManager::recording_replay) {
				PGGlobalReplayManager::RecordReadFile(filename, nullptr, 0, error);
			}
			return nullptr;
		}

		FILE* f = handle->f;
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* string = (char*)malloc(fsize + 1);
		if (!string) {
			error = PGFileIOError;
			if (PGGlobalReplayManager::recording_replay) {
				PGGlobalReplayManager::RecordReadFile(filename, nullptr, 0, error);
			}
			return nullptr;
		}
		fread(string, fsize, 1, f);
		CloseFile(handle);
		result_size = (lng)fsize;

		string[fsize] = 0;
		if (PGGlobalReplayManager::recording_replay) {
			PGGlobalReplayManager::RecordReadFile(filename, string, fsize, error);
		}
		return string;
	}

	void WriteToFile(PGFileHandle handle, const char* text, lng length) {
		if (PGGlobalReplayManager::running_replay) return;

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