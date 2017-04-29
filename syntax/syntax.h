#pragma once

#include "utils.h"
#include <vector>

typedef void* PGParserState;
typedef short PGSyntaxType;

extern const PGParserState PGParserErrorState;

extern const PGSyntaxType PGSyntaxError;
extern const PGSyntaxType PGSyntaxNone;
extern const PGSyntaxType PGSyntaxString;
extern const PGSyntaxType PGSyntaxConstant;
extern const PGSyntaxType PGSyntaxComment;
extern const PGSyntaxType PGSyntaxOperator;
extern const PGSyntaxType PGSyntaxFunction;
extern const PGSyntaxType PGSyntaxKeyword;
extern const PGSyntaxType PGSyntaxClass1;
extern const PGSyntaxType PGSyntaxClass2;
extern const PGSyntaxType PGSyntaxClass3;
extern const PGSyntaxType PGSyntaxClass4;
extern const PGSyntaxType PGSyntaxClass5;
extern const PGSyntaxType PGSyntaxClass6;

struct PGSyntaxNode {
	PGSyntaxType type;
	bool transparent = false;
	lng end = -1;

	PGSyntaxNode() : type(PGSyntaxNone), end(-1), transparent(false) { }
	PGSyntaxNode(PGSyntaxType type, lng end) : type(type), end(end), transparent(false) { }
};

struct PGSyntax {
	std::vector<PGSyntaxNode> syntax;
};
