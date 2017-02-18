
#include "findresults.h"

struct FindResultState {
	PGLanguage* language;
	std::unique_ptr<SyntaxHighlighter> highlighter;
	PGParserState parser_state;

	FindResultState() : language(nullptr), highlighter(nullptr), parser_state(nullptr) { }
};

PGParserState FindResultsHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState s, PGParseErrors& errors, PGSyntax& current) {
	FindResultState* state = (FindResultState*)s;
	char* text = line.GetLine();
	lng size = line.GetLength();
	// free the current syntax of this line
	current.syntax.clear();
	if (size > 5) {
		if (std::string(text, 5) == "File:") {
			current.syntax.push_back(PGSyntaxNode(PGSyntaxString, size));
			PGFile file = PGFile(std::string(text, size));
			if (state->highlighter) {
				state->highlighter->DeleteParserState(state->parser_state);
				state->highlighter = nullptr;
				state->parser_state = nullptr;
			}
			state->language = PGLanguageManager::GetLanguage(file.Extension());
			state->highlighter = state->language ? std::unique_ptr<SyntaxHighlighter>(state->language->CreateHighlighter()) : nullptr;
			state->parser_state = state->highlighter ? state->highlighter->GetDefaultState() : nullptr;
			return state;
		}
	}
	if (size > 2) {
		lng ptr = 0;
		while (ptr < size && text[ptr] >= '0' && text[ptr] <= '9') {
			ptr++;
		}
		PGSyntaxNode node;
		node.end = -1;
		if (ptr < size && text[ptr] == ':') {
			node = PGSyntaxNode(PGSyntaxConstant, ++ptr);
		}
		++ptr;
		if (state->highlighter && ptr < size) {
			TextLine shifted_line(text + ptr, size - ptr);
			state->highlighter->IncrementalParseLine(shifted_line, linenr, state->parser_state, errors, current);
			for (auto it = current.syntax.begin(); it != current.syntax.end(); it++) {
				it->end += ptr;
			}
		}
		if (node.end >= 0) {
			current.syntax.insert(current.syntax.begin(), node);
		}
	}
	return state;
}

PGParserState FindResultsHighlighter::GetDefaultState() {
	FindResultState* state = new FindResultState();
	state->language = nullptr;
	return state;
}

PGParserState FindResultsHighlighter::CopyParserState(const PGParserState inp) {
	FindResultState* original = (FindResultState*)inp;
	FindResultState* state = new FindResultState();
	state->language = original->language;
	state->highlighter = state->language ? std::unique_ptr<SyntaxHighlighter>(state->language->CreateHighlighter()) : nullptr;
	state->parser_state = state->highlighter ? state->highlighter->CopyParserState(original->parser_state) : nullptr;
	return state;
}

void FindResultsHighlighter::DeleteParserState(PGParserState a_) {
	FindResultState* a = (FindResultState*)a_;
	if (a->highlighter) {
		a->highlighter->DeleteParserState(a->parser_state);
	}
	delete a;
}

bool FindResultsHighlighter::StateEquivalent(const PGParserState a_, const PGParserState b_) {
	FindResultState* a = (FindResultState*)a_;
	FindResultState* b = (FindResultState*)b_;
	if (a->language != b->language) return false;
	if (!a->highlighter && !b->highlighter) return true;
	if (a->highlighter && b->highlighter) {
		return a->highlighter->StateEquivalent(a->parser_state, b->parser_state);
	}
	return false;
}
