
#pragma once

extern "C" {
	typedef void* PGIgnoreGlob;

	// create a gitignore-globset for a given directory
	extern PGIgnoreGlob PGCreateGlobForDirectory(const char* path);
	// returns whether or not a given path is ignored/should be skipped
	extern bool PGFileIsIgnored(PGIgnoreGlob, const char* path, bool is_directory);
	// destroys the given ignore glob
	extern void PGDestroyIgnoreGlob(PGIgnoreGlob);
}
