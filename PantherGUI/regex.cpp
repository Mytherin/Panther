
#include "regex.h"
#include "unicode.h"

#include <re2/re2.h>


using namespace re2;

struct PGRegex {
	bool is_regex;
	std::unique_ptr<RE2> regex;
	std::vector<lng> table;
	std::vector<lng> reverse_table;
	std::string needle;
	PGRegexFlags flags;
};

static std::vector<lng> PGPreprocessTextSearch(std::string& needle);
static PGRegexMatch PGTextSearch(PGRegexHandle handle, PGTextRange context, PGDirection direction);

PGRegexHandle PGCompileRegex(const std::string& pattern, bool is_regex, PGRegexFlags flags) {
	PGRegexHandle handle = new PGRegex();
	handle->is_regex = is_regex;
	handle->flags = flags;
	if (is_regex) {
		// compile "pattern" as a regex pattern
		RE2::Options options;
		options.set_case_sensitive(!(flags & PGRegexCaseInsensitive));
		options.set_utf8(true);
		options.set_log_errors(false);
		options.set_posix_syntax(true);
		options.set_perl_classes(true);
		options.set_word_boundary(true);
		options.set_one_line(false);
		RE2* regex = new RE2(pattern.c_str(), options);
		if (!regex) {
			// FIXME: report on the error
			delete handle;
			return nullptr;
		}
		handle->regex = std::unique_ptr<RE2>(regex);
	} else {
		// regular string search
		// perform preprocessing on the needle
		handle->needle = pattern;
		if (flags & PGRegexCaseInsensitive) {
			handle->needle = panther::tolower(handle->needle);
			if (handle->needle == pattern) {
				// case insensitive is toggled on, but there are no ASCII letters in the search pattern
				// toggle off case-insensitive search because it makes no difference
				handle->flags = PGRegexFlagsNone;
			}
		}
		handle->table = PGPreprocessTextSearch(handle->needle);
		std::string reverse_needle = handle->needle;
		std::reverse(reverse_needle.begin(), reverse_needle.end());
		handle->reverse_table = PGPreprocessTextSearch(reverse_needle);
	}
	return handle;
}

PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGTextRange context, PGDirection direction) {
	PGRegexMatch match;
	match.matched = false;
	if (!handle) {
		return match;
	}
	if (handle->is_regex) {
		PGTextRange subtext = context;
		bool find_last_match = direction == PGDirectionLeft;
		match.matched = handle->regex.get()->Match(context, subtext, RE2::UNANCHORED, match.groups, PGREGEX_MAXIMUM_MATCHES, find_last_match);
		return match;
	} else {
		return PGTextSearch(handle, context, direction);
	}
}

PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context, PGDirection direction) {
	assert(0);
	// FIXME: the return value cannot be correct because it points into the below PGTextBuffer
	// which will be deleted after the function exists
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

int PGRegexNumberOfCapturingGroups(PGRegexHandle handle) {
	if (!handle->is_regex) return -1;
	return handle->regex.get()->NumberOfCapturingGroups();
}

std::vector<lng> PGPreprocessTextSearch(std::string& needle) {
	if (needle.size() >= 1) {
		// initialize a table of UCHAR_MAX+1 elements to value needle.size()
		std::vector<lng> table(256, needle.size());
		/* Populate it with the analysis of the needle */
		/* But ignoring the last letter */
		for(size_t a = 0; a < needle.size() - 1; a++) {
			unsigned char index = needle[a];
			table[index] = needle.size() - 1 - a;
		}
		return table;
	}
	return std::vector<lng>();
}

PGRegexMatch PGTextSearch(PGRegexHandle handle, PGTextRange context, PGDirection direction) {
	// find regular text within a textrange
	// this is a modified implementation of Boyer-Moore-Horspool
	// that works on ranges of buffer ranges
	assert(handle);
	assert(!handle->is_regex);
	PGRegexMatch match;
	match.matched = false;

	bool case_insensitive = handle->flags & PGRegexCaseInsensitive;

	if (handle->needle.size() == 1) {
		// needle size = 1 so boyer-moore makes no sense 
		// as there are no prefixes/suffixes
		PGTextPosition position;
		if (case_insensitive) {
			// case insensitive, have to find either the lower or upper variant of the character
			position = direction == PGDirectionRight ? 
				context._memcasechr(handle->needle[0]) : 
				context._memcaserchr(handle->needle[0]);

		} else {
			// case sensitive; just use memchr/memrchr
			position = direction == PGDirectionRight ? 
				context._memchr(handle->needle[0]) : 
				context._memrchr(handle->needle[0]);
		}
		if (position.buffer != nullptr) {
			// found match, set the proper return value range
			match.matched = true;
			match.groups[0] = PGTextRange(position, position + 1);
		}
		return match;
	}

	// perform the actual search
	const std::string& needle = handle->needle;
	PGTextRange range = context;

	if (direction == PGDirectionRight) {
		// search in the right direction
		// last_needle_char is the last character of the needle
		// boyer-moore-horspool works by first checking if the character that is
		// needle.size() away matches the last character of the needle
		// if it does not, it skips ahead based on the current character
		// the skip distance depends on how often the current character occurs in the pattern
		// and is precomputed in PGPreprocessTextSearch for each possible character

		const unsigned char last_needle_char = needle.back();
		const unsigned char last_needle_char_upper = panther::chartoupper(last_needle_char);
	 
		PGTextPosition position;
		position.buffer = context.start_buffer;
		position.position = context.start_position;
		while(true) {
			const unsigned char character = *(position + (needle.size() - 1));
			if (!character) return match;

			if (last_needle_char == character || 
				(case_insensitive && character == last_needle_char_upper)) {
				// the character <needle size> away matches the last character of the needle
				// perform a comparison
				range.start_buffer = position.buffer;
				range.start_position = position.position;
				bool found_match = case_insensitive ? 
					range.ascii_strcasecmp(needle.c_str(), needle.size() - 1) == 0 : 
					range._memcmp(needle.c_str(), needle.size() - 1) == 0;
				if (found_match) {
					match.matched = true;
					match.groups[0] = PGTextRange(position, position + needle.size());
					return match;
				}
			}
	 
			if (!position.Offset(handle->table[case_insensitive ? panther::chartolower(character) : character])) {
				return match;
			}
		}
	} else {
		// search in the left direction
		// this is mostly identical to the search in the right direction
		// except just reversed; we scan the string right-to-left and
		// look for the first character of the needle first
		const unsigned char first_needle_char = needle.front();
		const unsigned char first_needle_char_upper = panther::chartoupper(first_needle_char);
	 
		PGTextPosition position;
		position.buffer = context.end_buffer;
		position.position = context.end_position;
		while(true) {
			PGTextPosition last_position = position - (needle.size() - 1);
			const unsigned char character = *last_position;
			if (!character) return match;

			if (first_needle_char == character || 
				(case_insensitive && character == first_needle_char_upper)) {
				// the character <needle size> away matches the last character of the needle
				// perform a comparison
				PGTextPosition second_last_position = last_position + 1;
				range.start_buffer = last_position.buffer;
				range.start_position = last_position.position;
				bool found_match = case_insensitive ? 
					range.ascii_strcasecmp(needle.c_str(), needle.size() - 1) == 0 : 
					range._memcmp(needle.c_str(), needle.size() - 1) == 0;
				if (found_match) {
					match.matched = true;
					match.groups[0] = PGTextRange(last_position, position + 1);
					return match;
				}
			}
	 
			if (!position.Offset(-handle->reverse_table[case_insensitive ? panther::chartolower(character) : character])) {
				return match;
			}
		}
	}
}

