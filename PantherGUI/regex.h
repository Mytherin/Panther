#pragma once

#include "utils.h"
#include "textbuffer.h"
#include "textposition.h"

struct PGRegex;
typedef PGRegex* PGRegexHandle;

typedef void PGMatchCallback(void* data, std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng initial_line);

#define PGREGEX_MAXIMUM_MATCHES 16

struct PGRegexMatch {
	bool matched;
	PGTextRange groups[PGREGEX_MAXIMUM_MATCHES];
};

enum PGRegexFlags {
	PGRegexFlagsNone,
	PGRegexCaseInsensitive
};

PGRegexHandle PGCompileRegex(const std::string& pattern, bool is_regex, PGRegexFlags);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGTextRange context, PGDirection direction);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, const char* data, lng size, PGDirection direction);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context, PGDirection direction);
int PGRegexNumberOfCapturingGroups(PGRegexHandle handle);
void PGDeleteRegex(PGRegexHandle handle);

