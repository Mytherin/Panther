#pragma once


typedef short PGParserState;
typedef short PGSyntaxType;

struct PGSyntax {
	PGSyntaxType type;
	int end;
	PGSyntax* next;

	PGSyntax() : type(0), end(-1), next(nullptr) { }
};