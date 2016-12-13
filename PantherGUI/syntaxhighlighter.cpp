
#include "syntaxhighlighter.h"

const PGParserState PGParserErrorState = nullptr;

const PGSyntaxType PGSyntaxError = -1;
const PGSyntaxType PGSyntaxNone = 0;

SyntaxHighlighter::~SyntaxHighlighter() {

}

PGParserState SyntaxHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors) {
	return PGParserErrorState;
}


PGParserState SyntaxHighlighter::GetDefaultState() {
	return PGParserErrorState;
}

PGParserState SyntaxHighlighter::CopyParserState(const PGParserState state) {
	return PGParserErrorState;
}

void SyntaxHighlighter::DeleteParserState(PGParserState state) {

}

bool SyntaxHighlighter::StateEquivalent(const PGParserState a, const PGParserState b) {
	return true;
}