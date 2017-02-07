
#include "regex.h"
#include "unicode.h"

#include <re2/re2.h>


using namespace re2;

struct PGRegex {
	std::unique_ptr<RE2> regex;
};

PGRegexHandle PGCompileRegex(std::string& pattern, bool is_regex, PGRegexFlags flags) {
	RE2::Options options;
	options.set_case_sensitive(!(flags & PGRegexCaseInsensitive));
	RE2* regex = new RE2(pattern.c_str(), options);
	if (!regex) {
		// FIXME: report on the error
		return nullptr;
	}
	PGRegexHandle handle = new PGRegex();
	handle->regex = std::unique_ptr<RE2>(regex);
	return handle;
}

PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGTextRange context, PGDirection direction) {
	PGRegexMatch match;
	match.matched = false;
	if (!handle) {
		return match;
	}

	PGTextRange subtext = context;
	bool find_last_match = direction == PGDirectionLeft;
	match.matched = handle->regex.get()->Match(context, subtext, RE2::UNANCHORED, match.groups, PGREGEX_MAXIMUM_MATCHES, find_last_match);
	return match;
}

PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context, PGDirection direction) {
	PGTextBuffer buffer;
	buffer.buffer = (char*) context.data();
	buffer.current_size = context.size();
	buffer.start_line = 0;
	buffer.prev = nullptr;
	buffer.next = nullptr;

	PGTextRange text;
	text.start_buffer = &buffer;
	text.start_position = 0;
	text.end_buffer = &buffer;
	text.end_position = buffer.current_size;
	PGRegexMatch match = PGMatchRegex(handle, text, direction);
	buffer.buffer = nullptr;
	buffer.current_size = 0;
	return match;
}

void PGDeleteRegex(PGRegexHandle handle) {
	delete handle;
}
