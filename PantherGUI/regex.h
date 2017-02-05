#pragma once

#include "utils.h"
#include "textbuffer.h"

struct PGRegex;
typedef PGRegex* PGRegexHandle;

struct PGRegexBounds {
	PGTextBuffer* start_buffer;
	lng start_position;
	PGTextBuffer* end_buffer;
	lng end_position;

	PGRegexBounds() { }
	PGRegexBounds(PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position) :
		start_buffer(start_buffer), start_position(start_position), end_buffer(end_buffer), end_position(end_position) {

	}
};

#define PGREGEX_MAXIMUM_MATCHES 16

struct PGRegexMatch {
	bool matched;
	PGRegexBounds groups[PGREGEX_MAXIMUM_MATCHES];
};

enum PGRegexFlags {
	PGRegexFlagsNone,
	PGRegexCaseInsensitive
};

PGRegexHandle PGCompileRegex(std::string& pattern, PGRegexFlags);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGRegexBounds context, PGDirection direction);
void PGDeleteRegex(PGRegexHandle handle);

