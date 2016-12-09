#pragma once

#include "syntaxhighlighter.h"

extern const PGParserState PGParserXMLComment;
extern const PGParserState PGParserXMLElementName;
extern const PGParserState PGParserXMLInsideElement;
extern const PGParserState PGParserXMLOpenAttribute;
extern const PGParserState PGParserXMLStartValue;
extern const PGParserState PGParserXMLOpenValue;

class XMLHighlighter : public SyntaxHighlighter {
public:
	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, PGParserState state);
	PGParserState BacktrackParseLine(TextLine& line);
};