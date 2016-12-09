
#include "syntaxhighlighter.h"

const PGParserState PGParserErrorState = -1;
const PGParserState PGParserDefaultState = 0;

const PGSyntaxType PGSyntaxError = -1;
const PGSyntaxType PGSyntaxNone = 0;

SyntaxHighlighter::~SyntaxHighlighter() {

}

PGParserState SyntaxHighlighter::IncrementalParseLine(TextLine& line, PGParserState state) {
	return PGParserErrorState;
}

PGParserState SyntaxHighlighter::BacktrackParseLine(TextLine& line) {
	return PGParserErrorState;
}
