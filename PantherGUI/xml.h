#pragma once

#include "syntaxhighlighter.h"

extern const PGSyntaxType PGXMLElementName;
extern const PGSyntaxType PGXMLAttributeName;
extern const PGSyntaxType PGXMLValue;
extern const PGSyntaxType PGXMLComment;
extern const PGSyntaxType PGXMLEmpty;

class XMLHighlighter : public SyntaxHighlighter {
public:
	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors);
	PGParserState GetDefaultState();
	PGParserState CopyParserState(const PGParserState state);
	bool StateEquivalent(const PGParserState a, const PGParserState b);
	void DeleteParserState(PGParserState state);
};