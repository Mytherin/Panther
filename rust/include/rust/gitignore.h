
#pragma once

extern "C" {
	typedef void (*PGDirectoryCallback)(void* data, const char* path, bool is_directory);

	int PGListAllFiles(const char* directory, PGDirectoryCallback callback, void* data, bool relative_paths);
	int PGListFiles(const char* directory, PGDirectoryCallback callback, void* data, bool relative_paths);
}
