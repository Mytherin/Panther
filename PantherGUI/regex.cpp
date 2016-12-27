
#include "regex.h"
#include <unicode/regex.h>
#include <unicode/utf8.h>
#include "unicode.h"

struct PGRegex {
	bool matched = false;
	RegexMatcher* matcher = nullptr;
};

PGRegexHandle PGCompileRegex(std::string& pattern, PGRegexFlags flags) {
	UErrorCode status = U_ZERO_ERROR;
	uint32_t regex_flags = flags == PGRegexCaseInsensitive ? UREGEX_CASE_INSENSITIVE : 0;
	RegexMatcher *matcher = new RegexMatcher(UnicodeString::fromUTF8(pattern.c_str()), regex_flags, status);
	if (U_FAILURE(status)) {
		// FIXME: return reason why the regex failed
		return nullptr;
	}
	PGRegexHandle handle = new PGRegex();
	handle->matcher = matcher;
	handle->matched = false;
	return handle;
}

PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& string, PGDirection direction) {
	UErrorCode status = U_ZERO_ERROR;
	PGRegexMatch match;
	handle->matched = false;
	handle->matcher->reset(UnicodeString::fromUTF8(string.c_str()));
	int start, end;
	if (direction == PGDirectionRight) {
		// find the next match of the regex in the string, if any
		if (!handle->matcher->find()) {
			match.matched = false;
			return match;
		} else {
			match.matched = true;
			handle->matched = true;
			start = handle->matcher->start(status);
			end = handle->matcher->end(status);
		}
	} else {
		// find the right-most match
		// we do this naively now: just keep on matching until we no longer match
		// the resulting match is the last match
		match.matched = false;
		while (handle->matcher->find()) {
			match.matched = true;
			start = handle->matcher->start(status);
			end = handle->matcher->end(status);
		}
		if (!match.matched) {
			return match;
		}
		match.matched = true;
		handle->matched = true;
	}
	// the returned (start,end) characters are character numbers
	// we want to return the position in the UTF8 string as (start,end)
	// thus we scan the string to map from character number -> position in string
	int i = 0;
	int character_number = 0;
	while (true) {
		int offset = utf8_character_length(string[i]);
		if (character_number == start) {
			match.start = i;
		}
		if (character_number == end) {
			match.end = i;
			break;
		}
		if (i >= string.size()) break;
		character_number++;
		i += offset;
	}
	return match;
}

PGRegexMatch PGMatchRegex(std::string& pattern, std::string& string, PGDirection direction, PGRegexFlags flags) {
	PGRegexHandle handle = PGCompileRegex(pattern, flags);
	PGRegexMatch result = PGMatchRegex(handle, string, direction);
	PGDeleteRegex(handle);
	return result;
}

char* PGGetRegexGroup(PGRegexHandle handle, int groupnr) {
	UErrorCode status = U_ZERO_ERROR;
	if (!handle->matched) return nullptr;
	auto text = handle->matcher->group(status);
	if (U_FAILURE(status)) {
		// FIXME: return reason for failure
		return nullptr;
	}
	std::string result;
	text.toUTF8String(result);
	return _strdup(result.c_str());
}

void PGDeleteRegex(PGRegexHandle handle) {
	delete handle->matcher;
	delete handle;
}
