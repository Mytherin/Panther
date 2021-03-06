
#include "keywords.h"
#include "unicode.h"

struct KWParserState {
	PGParserKWState state;
	std::string end;
	char escape;
};

static bool KeywordCharacter(char c) {
	return c == '#' || c == '_' || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57);
} 

PGParserState KeywordHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState s, PGParseErrors& errors, PGSyntax& current) {
	assert(is_generated);
	KWParserState* state = (KWParserState*)s;
	char* text = line.GetLine();
	lng size = line.GetLength();
	// free the current syntax of this line
	current.syntax.clear();
	bool escaped = false;
	// parse the actual keywords
	for (lng i = 0; i < size; i++) {
		int utf8_length = utf8_character_length(text[i]);
		if (utf8_length > 1) {
			// special characters are not supported in the keyword highlighter (for now)
			i += utf8_length - 1;
			continue;
		}
		if (state->state == PGParserKWDefault) {
			if (KeywordCharacter(text[i])) {
				lng start = i;
				// consume keyword
				while (i < size && KeywordCharacter(text[i]))
					i++;
				std::string kw = std::string(text + start, i - start);
				if (keyword_index.find(kw) != keyword_index.end()) {
 					PGSyntaxType type = keyword_index[kw];
					if (start > 0 && 
						(current.syntax.size() == 0 || current.syntax.back().end < start)) {
						current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, start));
					}
					current.syntax.push_back(PGSyntaxNode(type, i));
				}
			} else {
				char entry = text[i];
				if (token_index.find(entry) != token_index.end()) {
					std::vector<PGSyntaxPair>& tokens = token_index[entry];
					for(auto it = tokens.begin(); it != tokens.end(); it++) {
						std::string& start = it->start;
						if (i + start.size() > size) {
							continue;
						}
						bool found = true;
						for(lng j = 0; j < start.size(); j++) {
							if (text[i + j] != start[j]) {
								found = false;
								break;
							}
						}
						if (found) {
							if (i > 0 && 
								(current.syntax.size() == 0 || current.syntax.back().end < i)) {
								current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, i));
							}
							state->end = it->end;
							state->state = it->state;
							state->escape = it->escape;
							i = i + start.size() - 1;
							if (state->end.size() == 0) {
								i = size - 1;
								break;
							}
						}
					}
				}
			}
		} else {
			if (!escaped) {
				if (text[i] == state->escape) {
					escaped = true;
					continue;
				}
				if (i + state->end.size() > size) {
					i = size - 1;
					break;
				}
				bool found = true;
				for(lng j = 0; j < state->end.size(); j++) {
					if (text[i + j] != state->end[j]) {
						found = false;
						break;
					}
				}
				if (found) {
					i = i + state->end.size() - 1;
					PGSyntaxType type = (state->state == PGParserKWSLComment || state->state == PGParserKWMLComment) ? PGSyntaxComment : PGSyntaxString;
					current.syntax.push_back(PGSyntaxNode(type, i + 1));
					state->state = PGParserKWDefault;
					state->end = "";
					state->escape = '\0';
				}
			} else {
				escaped = false;
			}
		}
	}
	if (state->state == PGParserKWSLComment || state->state == PGParserKWMLComment) {
		current.syntax.push_back(PGSyntaxNode(PGSyntaxComment, size));
		if (state->state == PGParserKWSLComment) {
			if (state->end.size() != 0) {
				errors.errors.push_back(PGParseError(size - 1, size - 1, linenr, "Expected " + state->end));
			}
			state->state = PGParserKWDefault;
			state->end = "";
			state->escape = '\0';
		}
	} else if (state->state == PGParserKWSLString || state->state == PGParserKWMLString) {
		current.syntax.push_back(PGSyntaxNode(PGSyntaxString, size));
		if (state->state == PGParserKWSLString) {
			if (state->end.size() != 0) {
				errors.errors.push_back(PGParseError(size - 1, size - 1, linenr, "Expected " + state->end));
			}
			state->state = PGParserKWDefault;
			state->end = "";
			state->escape = '\0';
		}
	}
	return state;
}

PGParserState KeywordHighlighter::GetDefaultState() {
	KWParserState* state = new KWParserState();
	state->state = PGParserKWDefault;
	state->end = "";
	state->escape = '\0';
	return state;
}

PGParserState KeywordHighlighter::CopyParserState(const PGParserState inp) {
	KWParserState* original = (KWParserState*)inp;
	KWParserState* state = new KWParserState();
	state->state = original->state;
	state->end = original->end;
	state->escape = original->escape;
	return state;
}

bool KeywordHighlighter::StateEquivalent(const PGParserState aa, const PGParserState bb) {
	KWParserState* a = (KWParserState*)aa;
	KWParserState* b = (KWParserState*)bb;
	return (a->state == PGParserKWDefault && b->state == PGParserKWDefault) ? true : 
		(a->state == b->state && a->escape == b->escape && a->end == b->end);
}

void KeywordHighlighter::DeleteParserState(PGParserState state) {
	delete (KWParserState*) state;
}

void KeywordHighlighter::GenerateIndex(
	std::vector<PGKWEntry>& keywords,
	std::vector<PGSyntaxPair>& specials) {
	for(auto it = keywords.begin(); it != keywords.end(); it++) {
		keyword_index[it->entry] = it->result_state;
	}
	for(auto it = specials.begin(); it != specials.end(); it++) {
		if (it->start.size() == 0) continue;
		char c = it->start[0];
		if (token_index.find(c) == token_index.end()) {
			token_index[c] = std::vector<PGSyntaxPair>();
		}
		token_index[c].push_back(*it);
	}
	this->is_generated = true;
}
