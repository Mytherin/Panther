#pragma once


typedef short PGParserState;
typedef short PGSyntaxType;

struct PGSyntax {
	PGSyntaxType type;
	int end;
	PGSyntax* next;

	PGSyntax() : end(-1), next(nullptr) { }
};