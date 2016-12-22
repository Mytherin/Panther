
#include "syntaxhighlighter.h"

const PGParserState PGParserErrorState = nullptr;

const PGSyntaxType PGSyntaxError = -1;
const PGSyntaxType PGSyntaxNone = 0;
const PGSyntaxType PGSyntaxString = 1;
const PGSyntaxType PGSyntaxConstant = 2;
const PGSyntaxType PGSyntaxComment = 3;
const PGSyntaxType PGSyntaxOperator = 4;
const PGSyntaxType PGSyntaxFunction = 5;
const PGSyntaxType PGSyntaxKeyword = 6;
const PGSyntaxType PGSyntaxClass1 = 7;
const PGSyntaxType PGSyntaxClass2 = 8;
const PGSyntaxType PGSyntaxClass3 = 9;
const PGSyntaxType PGSyntaxClass4 = 10;
const PGSyntaxType PGSyntaxClass5 = 11;
const PGSyntaxType PGSyntaxClass6 = 12;

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