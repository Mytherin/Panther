#pragma once

#include "utils.h"

struct PGRegex;
typedef PGRegex* PGRegexHandle;

struct PGRegexMatch {
	bool matched;
	int start;
	int end;
};

enum PGRegexFlags {
	PGRegexFlagsNone,
	PGRegexCaseInsensitive
};

PGRegexHandle PGCompileRegex(std::string& pattern, PGRegexFlags);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& string, PGDirection direction);
PGRegexMatch PGMatchRegex(std::string& pattern, std::string& string, PGDirection direction, PGRegexFlags);
char* PGGetRegexGroup(PGRegexHandle handle, int groupnr);
void PGDeleteRegex(PGRegexHandle handle);

