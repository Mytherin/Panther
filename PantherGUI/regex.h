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

PGRegexHandle PGCompileRegex(std::string& pattern, PGRegexFlags);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGTextRange context);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context);
void PGDeleteRegex(PGRegexHandle handle);

