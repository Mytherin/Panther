#pragma once


typedef short PGParserState;
typedef short PGSyntaxType;

extern const PGParserState PGParserUnknownState;
extern const PGParserState PGParserErrorState;
extern const PGParserState PGParserDefaultState;

extern const PGSyntaxType PGSyntaxError;
extern const PGSyntaxType PGSyntaxNone;

struct PGSyntax {
	PGSyntaxType type;
	int end;
	PGSyntax* next;

	PGSyntax() : type(0), end(-1), next(nullptr) { }
};
