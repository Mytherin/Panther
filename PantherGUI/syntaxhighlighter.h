#pragma once

#include "syntax.h"
#include "textline.h"

enum SyntaxHighlighterType {
	//! Implements no methods
	PGSyntaxHighlighterNone,
	//! Basic syntax highlighter, only provides "IncrementalParseLine" method
	PGSyntaxHighlighterIncremental,
	//! Backtracking syntax highlighter, provides "BacktrackParseLine" method
	PGSyntaxHighlighterBacktracking,
};

extern const PGParserState PGParserErrorState;
extern const PGParserState PGParserDefaultState;

extern const PGSyntaxType PGSyntaxError;
extern const PGSyntaxType PGSyntaxNone;

class SyntaxHighlighter {
public:
	virtual ~SyntaxHighlighter();
	// Returns the type of the syntax highlighter, which indicates the supported operations
	virtual SyntaxHighlighterType GetType() { return PGSyntaxHighlighterNone; }
	// Parses a line with a given input state, places the parsed tokens in the TextLine and returns an output state
	// This is the basic function that every syntax highlighter should provide
	// Parsers that only provide this function require the entire file to be parsed
	virtual PGParserState IncrementalParseLine(TextLine& line, PGParserState state);
	// Parses a line without input state by backtracking
	// This allows for more efficient parsing of large files because the entire file does not need to be parsed
	// Instead, the parser can backtrack from the current viewpoint 
	virtual PGParserState BacktrackParseLine(TextLine& line);
};