#pragma once

#include "syntax.h"
#include "textline.h"

enum SyntaxHighlighterType {
	//! Implements no methods
	PGSyntaxHighlighterNone,
	//! Basic syntax highlighter, only provides "IncrementalParseLine" method
	PGSyntaxHighlighterIncremental,
};

class SyntaxHighlighter {
public:
	virtual ~SyntaxHighlighter();
	// Returns the type of the syntax highlighter, which indicates the supported operations
	virtual SyntaxHighlighterType GetType() { return PGSyntaxHighlighterNone; }
	// Parses a line with a given input state, places the parsed tokens in the TextLine and returns an output state
	// This is the basic function that every syntax highlighter should provide
	// Parsers that only provide this function require the entire file to be parsed
	virtual PGParserState IncrementalParseLine(TextLine& line, PGParserState state);
};
