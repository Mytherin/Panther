
#include "syntect.h"


struct SyntectParserState {
	PGSyntaxState state;
};

SyntectHighlighter::SyntectHighlighter() {
	this->parser = PGCreateParser();
}

SyntectHighlighter::~SyntectHighlighter() {
	PGDestroyParser(this->parser);
}

PGParserState SyntectHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax) {
	SyntectParserState* s = static_cast<SyntectParserState*>(state);
	PGParserState _new = PGParseLine(this->parser, s->state, line.GetLine());
	return _new;
}

PGParserState SyntectHighlighter::GetDefaultState() {
	SyntectParserState* s = new SyntectParserState();
	s->state = PGGetDefaultState(this->parser);
	return (PGParserState) s;
}

PGParserState SyntectHighlighter::CopyParserState(const PGParserState state) {
	SyntectParserState* s = static_cast<SyntectParserState*>(state);
	SyntectParserState* _new = new SyntectParserState();
	_new->state = PGCopyParserState(s->state);
	return _new;
}

bool SyntectHighlighter::StateEquivalent(const PGParserState a, const PGParserState b) {
	SyntectParserState* _a = static_cast<SyntectParserState*>(a);
	SyntectParserState* _b = static_cast<SyntectParserState*>(b);
	return PGStateEquivalent(_a->state, _b->state);
}

void SyntectHighlighter::DeleteParserState(PGParserState state) {
	if (!state) return;

	SyntectParserState* s = static_cast<SyntectParserState*>(state);
	PGDestroyParserState(s->state);
	delete s;
}
