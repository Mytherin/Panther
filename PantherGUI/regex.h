#pragma once

#include "utils.h"
#include "textbuffer.h"
#include "textposition.h"

struct PGRegex;
typedef PGRegex* PGRegexHandle;

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
PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context, PGDirection direction);
int PGRegexNumberOfCapturingGroups(PGRegexHandle handle);
void PGDeleteRegex(PGRegexHandle handle);

