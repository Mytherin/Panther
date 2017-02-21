
#pragma once

extern "C" {
	typedef void (*PGDirectoryCallback)(void* data, const char* path);

	int PGListFiles(const char* directory, PGDirectoryCallback callback, void* data);
}
