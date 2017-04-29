#pragma once

#include "keywords.h"
#include "language.h"

class FindResultsHighlighter : public SyntaxHighlighter {
public:
	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax);
	PGParserState GetDefaultState();
	PGParserState CopyParserState(const PGParserState state);
	bool StateEquivalent(const PGParserState a, const PGParserState b);
	void DeleteParserState(PGParserState state);
};

class FindResultsLanguage : public PGLanguage {
	std::string GetName() { return "Find Results"; }
	SyntaxHighlighter* CreateHighlighter() { return new FindResultsHighlighter(); }
	bool MatchesFileExtension(std::string extension) { return extension == "findresults"; }
	std::string GetExtension() { return "Find"; }
	PGColor GetColor() { return PGColor(255, 255, 255); }
};
