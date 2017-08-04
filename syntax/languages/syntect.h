#pragma once

#include "syntaxhighlighter.h"
#include "language.h"
#include "rust/syntax.h"

class SyntectHighlighter : public SyntaxHighlighter {
public:
	SyntectHighlighter();
	~SyntectHighlighter();

	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax);
	PGParserState GetDefaultState();
	PGParserState CopyParserState(const PGParserState state);
	bool StateEquivalent(const PGParserState a, const PGParserState b);
	void DeleteParserState(PGParserState state);
private:
	PGSyntaxParser parser;
};
