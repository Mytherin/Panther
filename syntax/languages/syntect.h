#pragma once

#include "syntaxhighlighter.h"
#include "language.h"

class SyntectHighlighter : public SyntaxHighlighter {
public:
	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax);
	PGParserState GetDefaultState();
	PGParserState CopyParserState(const PGParserState state);
	bool StateEquivalent(const PGParserState a, const PGParserState b);
	void DeleteParserState(PGParserState state);
};
