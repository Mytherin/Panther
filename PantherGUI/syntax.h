#pragma once

#include "utils.h"

typedef void* PGParserState;
typedef short PGSyntaxType;

extern const PGParserState PGParserErrorState;

extern const PGSyntaxType PGSyntaxError;
extern const PGSyntaxType PGSyntaxNone;

struct PGSyntax {
	PGSyntaxType type;
	ssize_t end;
	PGSyntax* next;

	PGSyntax() : type(0), end(-1), next(nullptr) { }
	~PGSyntax() { }
};
