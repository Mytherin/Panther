#pragma once

#include "syntax.h"
#include "textbuffer.h"
#include "textline.h"

#include <vector>

enum SyntaxHighlighterType {
	//! Implements no methods
	PGSyntaxHighlighterNone,
	//! Basic syntax highlighter, only provides "IncrementalParseLine" method
	PGSyntaxHighlighterIncremental,
};

struct PGParseError {
	lng start;
	lng end;
	lng linenr;
	std::string message;

	PGParseError(lng start, lng end, lng linenr, std::string message) : start(start), end(end), linenr(linenr), message(message) { }
};

struct PGParseErrors {
	std::vector<PGParseError> errors;
};

class SyntaxHighlighter {
public:
	virtual ~SyntaxHighlighter();
	// Returns the type of the syntax highlighter, which indicates the supported operations
	virtual SyntaxHighlighterType GetType() { return PGSyntaxHighlighterNone; }
	// Parses a line with a given input state, places the parsed tokens in the TextLine and returns an output state
	// This is the basic function that every syntax highlighter should provide
	// Parsers that only provide this function require the entire file to be parsed
	virtual PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors);
	// Returns the initial parser state
	virtual PGParserState GetDefaultState();
	// Returns a copy of the specified parser state
	virtual PGParserState CopyParserState(const PGParserState state);
	// Deletes the specified parser state
	virtual void DeleteParserState(PGParserState state);
	// Returns true if two states are equivalent, and false otherwise
	virtual bool StateEquivalent(const PGParserState a, const PGParserState b);
};
