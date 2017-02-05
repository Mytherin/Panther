
#include "regex.h"
#include "unicode.h"

#include <re2/re2.h>


using namespace re2;

struct PGRegex {
	std::unique_ptr<RE2> regex;
};

static PGRegexContext PGContextFromBounds(PGRegexBounds in) {
	PGRegexContext out;
	out.start_buffer = in.start_buffer;
	out.start_position = in.start_position;
	out.end_buffer = in.end_buffer;
	out.end_position = in.end_position;
	return out;
}

static PGRegexBounds PGBoundsFromContext(PGRegexContext in) {
	PGRegexBounds out;
	out.start_buffer = in.start_buffer;
	out.start_position = in.start_position;
	out.end_buffer = in.end_buffer;
	out.end_position = in.end_position;
	return out;
}

PGRegexHandle PGCompileRegex(std::string& pattern, PGRegexFlags flags) {
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

PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGRegexBounds bounds, PGDirection direction) {
	PGRegexMatch match;
	match.matched = false;
	if (!handle) {
		return match;
	}

	PGRegexContext context = PGContextFromBounds(bounds);
	PGRegexContext subtext = context;
	PGRegexContext matches[PGREGEX_MAXIMUM_MATCHES];
	match.matched = handle->regex.get()->Match(context, subtext, RE2::UNANCHORED, matches, PGREGEX_MAXIMUM_MATCHES);
	if (match.matched) {
		for (int i = 0; i < PGREGEX_MAXIMUM_MATCHES; i++) {
			match.groups[i] = PGBoundsFromContext(matches[i]);
		}
	}
	return match;
}

void PGDeleteRegex(PGRegexHandle handle) {
	delete handle;
}
