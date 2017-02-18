
#pragma once

extern "C" {
	typedef void* PGGlobBuilder;
	typedef void* PGGlobSet;

	// glob builders can be used to create a globset from multiple different glob patterns
	extern PGGlobBuilder PGCreateGlobBuilder(void);
	// destroy a glob builder created through PGCreateGlobBuilder()
	extern void PGDestroyGlobBuilder(PGGlobBuilder);
	// adds a glob (e.g. "*.c") to the glob builder. Returns 0 on success, or -1 on failure.
	extern int PGGlobBuilderAddGlob(PGGlobBuilder, const char* glob);

	// compiles a glob builder into a glob set that can be used to match globs
	extern PGGlobSet PGCompileGlobBuilder(PGGlobBuilder);
	// returns true if the given glob set matches the text
	extern bool PGGlobSetMatches(PGGlobSet, const char* text);
	// destroys a glob set created through PGCompileGlobBuilder
	extern void PGDestroyGlobSet(PGGlobSet);
}
