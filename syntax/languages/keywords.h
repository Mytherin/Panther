#pragma once

#include "syntaxhighlighter.h"
#include "utils.h"
#include <map>

enum PGParserKWState {
	PGParserKWDefault,
	PGParserKWMLComment,
	PGParserKWSLComment,
	PGParserKWMLString,
	PGParserKWSLString
};

struct PGSyntaxPair {
	std::string start;
	std::string end;
	PGParserKWState state;
	char escape;

	PGSyntaxPair(std::string start, std::string end, PGParserKWState state, char escape) : start(start), end(end), state(state), escape(escape) { }
};

struct PGKWEntry {
	std::string entry;
	PGSyntaxType result_state;

	PGKWEntry(std::string entry, PGSyntaxType result_state) : entry(entry), result_state(result_state) { }
};

class KeywordHighlighter : public SyntaxHighlighter {
public:
	virtual PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax);
	virtual PGParserState GetDefaultState();
	virtual PGParserState CopyParserState(const PGParserState state);
	virtual bool StateEquivalent(const PGParserState a, const PGParserState b);
	virtual void DeleteParserState(PGParserState state);
protected:
	void GenerateIndex(std::vector<PGKWEntry>& keywords, std::vector<PGSyntaxPair>& specials);
private:
	bool is_generated;
	std::map<std::string, PGSyntaxType> keyword_index;
	std::map<char, std::vector<PGSyntaxPair>> token_index;
};
