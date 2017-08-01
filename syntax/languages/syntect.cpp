
#include "syntect.h"
#include "rust/syntax.h"


struct SyntectParserState {
	PGParserSyntectState state;
};

PGParserState SyntectHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax) {

}

PGParserState SyntectHighlighter::GetDefaultState() {

}

PGParserState SyntectHighlighter::CopyParserState(const PGParserState state) {

}

bool SyntectHighlighter::StateEquivalent(const PGParserState a, const PGParserState b) {

}

void SyntectHighlighter::DeleteParserState(PGParserState state) {

}
